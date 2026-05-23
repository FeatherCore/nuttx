/****************************************************************************
 * arch/arm/src/armv8-m/arm_svcall.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <inttypes.h>
#include <stdint.h>
#include <assert.h>
#include <nuttx/debug.h>
#include <syscall.h>

#include <arch/irq.h>
#include <arch/barriers.h>
#include <nuttx/macro.h>
#include <nuttx/sched.h>
#include <nuttx/userspace.h>
#ifdef CONFIG_ARMV8M_SYSCALL_KERNEL_STACK
#  include <nuttx/kmalloc.h>
#endif

#include "sched/sched.h"
#include "signal/signal.h"
#include "exc_return.h"
#include "arm_control.h"
#include "arm_internal.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#if defined(CONFIG_LIB_SYSCALL) && defined(CONFIG_ARMV8M_SYSCALL_KERNEL_STACK)
static void arm_syscall_copy_frame(FAR uint32_t *dest, FAR const uint32_t *src,
                                   size_t nwords)
{
  FAR volatile uint32_t *vdest = dest;
  FAR volatile const uint32_t *vsrc = src;

  /* Keep this copy as a plain word loop.  The optimized libc memcpy path can
   * use MVE/FP instructions on Armv8-M, which would create lazy-FPU state
   * while the SVC handler is still arranging the exception frame.
   */

  while (nwords-- > 0)
    {
      *vdest++ = *vsrc++;
    }
}

static size_t arm_syscall_hw_frame_size(uint32_t excreturn)
{
#ifdef CONFIG_ARCH_FPU
  /* EXC_RETURN_STD_CONTEXT clear means hardware stacked the extended FP
   * frame.  The copied frame must match the exact active stack layout or the
   * exception return will unstack from the wrong address.
   */

  if ((excreturn & EXC_RETURN_STD_CONTEXT) == 0)
    {
      return HW_XCPT_SIZE;
    }
#endif

  return HW_INT_REGS * sizeof(uint32_t);
}

static uint32_t *arm_syscall_kstack_frame(FAR struct tcb_s *rtcb,
                                          FAR uint32_t *regs,
                                          uint32_t excreturn)
{
  size_t hwsize = arm_syscall_hw_frame_size(excreturn);
  size_t framesize = SW_XCPT_SIZE + hwsize;
  uintptr_t top;
  FAR uint32_t *kregs;

  /* Free any stacks retired by previous task exits before possibly allocating
   * another one.  They cannot be freed from up_release_stack() because the
   * exiting task can still be unwinding on its syscall stack.
   */

  armv8m_syscall_kstack_drain();

  if (rtcb->xcp.kstack == NULL)
    {
      rtcb->xcp.kstack =
        kmm_memalign(8, CONFIG_ARMV8M_SYSCALL_KERNEL_STACKSIZE);
      if (rtcb->xcp.kstack == NULL)
        {
          return regs;
        }
    }

  top = (uintptr_t)rtcb->xcp.kstack +
        CONFIG_ARMV8M_SYSCALL_KERNEL_STACKSIZE;
  top &= ~(uintptr_t)7;

  kregs = (FAR uint32_t *)(top - framesize);
  arm_syscall_copy_frame(kregs, regs, framesize / sizeof(uint32_t));

  /* The copied context is now going to run privileged syscall code on the
   * internal stack.  Keep the original user PSP frame in ustkptr so
   * SYS_syscall_return can restore it after the kernel stub finishes.
   */

#ifdef CONFIG_ARMV8M_STACKCHECK_HARDWARE
  /* Hardware stack-limit checking follows the active PSP/MSP, so the copied
   * frame needs the syscall stack bounds rather than the original user stack
   * bounds.
   */

  kregs[REG_SPLIM] = (uint32_t)(uintptr_t)rtcb->xcp.kstack;
#endif

  kregs[REG_SP] = (uint32_t)top;
  rtcb->xcp.ustkptr = regs;
  rtcb->xcp.regs = kregs;

  return kregs;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: arm_svcall
 *
 * Description:
 *   This is SVCall exception handler that performs context switching
 *
 ****************************************************************************/

int arm_svcall(int irq, void *context, void *arg)
{
  uint32_t *regs = (uint32_t *)context;
  struct tcb_s *tcb;
  uint32_t cmd;

  cmd = regs[REG_R0];

  /* The SVCall software interrupt is called with R0 = system call command
   * and R1..R7 =  variable number of arguments depending on the system call.
   */

#ifdef CONFIG_DEBUG_SYSCALL_INFO
#  ifndef CONFIG_DEBUG_SVCALL
  if (cmd > SYS_switch_context)
#  endif
    {
      svcinfo("SVCALL Entry: regs: %p cmd: %d\n", regs, cmd);
      svcinfo("  R0: %08x %08x %08x %08x %08x %08x %08x %08x\n",
              regs[REG_R0],  regs[REG_R1],  regs[REG_R2],  regs[REG_R3],
              regs[REG_R4],  regs[REG_R5],  regs[REG_R6],  regs[REG_R7]);
      svcinfo("  R8: %08x %08x %08x %08x %08x %08x %08x %08x\n",
              regs[REG_R8],  regs[REG_R9],  regs[REG_R10], regs[REG_R11],
              regs[REG_R12], regs[REG_R13], regs[REG_R14], regs[REG_R15]);
      svcinfo(" PSR: %08x EXC_RETURN: %08x CONTROL: %08x\n",
              regs[REG_XPSR], regs[REG_EXC_RETURN], regs[REG_CONTROL]);
    }
#endif

  /* Handle the SVCall according to the command in R0 */

  switch (cmd)
    {
      case SYS_restore_context:
      case SYS_switch_context:
        {
          tcb = this_task();
          restore_critical_section(tcb, this_cpu());

#ifdef CONFIG_DEBUG_SYSCALL_INFO
          regs = tcb->xcp.regs;
#endif
        }
        break;

      /* R0=SYS_syscall_return:  This a syscall return command:
       *
       *   void arm_syscall_return(void);
       *
       * At this point, the following values are saved in context:
       *
       *   R0 = SYS_syscall_return
       *
       * We need to restore the saved return address and return in
       * unprivileged thread mode.
       */

#ifdef CONFIG_LIB_SYSCALL
      case SYS_syscall_return:
        {
          struct tcb_s *rtcb = this_task();
          int index = (int)rtcb->xcp.nsyscalls - 1;
          uint32_t *retregs = regs;
          uint32_t excreturn;
          uint32_t control;
          uint32_t ret0;
          uint32_t ret1;

          /* Make sure that there is a saved syscall return address. */

          DEBUGASSERT(index >= 0);

          excreturn = rtcb->xcp.syscall[index].excreturn;
          control   = rtcb->xcp.syscall[index].ctrlreturn;
          ret0      = regs[REG_R2];
          ret1      = regs[REG_R1];

#ifdef CONFIG_ARMV8M_SYSCALL_RETURN_CURRENT_FRAME
#  ifdef CONFIG_ARMV8M_SYSCALL_KERNEL_STACK
          if (!(index == 0 && rtcb->xcp.ustkptr != NULL))
#  endif
          {
            /* Preserve the frame shape of the active SVC-return frame.  Under
             * lazy FPU, restoring an old EXC_RETURN FType bit into the current
             * frame can make hardware unstack a basic frame as extended, or
             * the reverse.
             */

            excreturn = (excreturn & ~EXC_RETURN_STD_CONTEXT) |
                        (regs[REG_EXC_RETURN] & EXC_RETURN_STD_CONTEXT);
          }
#endif

          control   = arm_control_set_mode(control, excreturn,
                                           (control & CONTROL_NPRIV) != 0);

#ifdef CONFIG_ARMV8M_SYSCALL_KERNEL_STACK
          if (index == 0 && rtcb->xcp.ustkptr != NULL)
            {
              /* The privileged dispatcher ran on the internal syscall stack.
               * Return the syscall result through the saved user PSP frame
               * and make that frame the task's live register context again.
               */

              retregs = rtcb->xcp.ustkptr;
              rtcb->xcp.ustkptr = NULL;
              rtcb->xcp.regs = retregs;
            }
#endif

          /* Setup to return to the saved syscall return address in
           * the original mode.
           */

          retregs[REG_PC]         = rtcb->xcp.syscall[index].sysreturn;
          retregs[REG_EXC_RETURN] = excreturn;
          retregs[REG_CONTROL]    = control;
#ifdef CONFIG_ARMV8M_SYSCALL_RETURN_USER_BASEPRI0
          if ((control & CONTROL_NPRIV) != 0)
            {
              /* User code must not resume with a BASEPRI mask inherited from
               * a kernel critical section or a blocking syscall wakeup path.
               */

              retregs[REG_BASEPRI] = 0;
            }
#endif
          rtcb->xcp.nsyscalls  = index;

          /* The return value must be in R0-R1.  arm_dispatch_syscall()
           * temporarily moved the value for R0 into R2.
           */

          retregs[REG_R0]      = ret0;
          retregs[REG_R1]      = ret1;
          regs                 = retregs;

          /* Handle any signal actions that were deferred while processing
           * the system call.
           */

          rtcb->flags          &= ~TCB_FLAG_SYSCALL;
          nxsig_unmask_pendingsignal();
        }
        break;
#endif

      /* R0=SYS_task_start:  This a user task start
       *
       *   void up_task_start(main_t taskentry, int argc, char *argv[])
       *          noreturn_function;
       *
       * At this point, the following values are saved in context:
       *
       *   R0 = SYS_task_start
       *   R1 = taskentry
       *   R2 = argc
       *   R3 = argv
       */

#ifdef CONFIG_BUILD_PROTECTED
      case SYS_task_start:
        {
          /* Set up to return to the user-space task start-up function in
           * unprivileged mode.
           */

          regs[REG_PC]         = (uint32_t)USERSPACE->task_startup & ~1;
          regs[REG_EXC_RETURN] = EXC_RETURN_THREAD;

          /* Return unprivileged mode */

          regs[REG_CONTROL]    =
            arm_control_set_mode(regs[REG_CONTROL], regs[REG_EXC_RETURN],
                                 true);

          /* Change the parameter ordering to match the expectation of struct
           * userpace_s task_startup:
           */

          regs[REG_R0]         = regs[REG_R1]; /* Task entry */
          regs[REG_R1]         = regs[REG_R2]; /* argc */
          regs[REG_R2]         = regs[REG_R3]; /* argv */
        }
        break;
#endif

      /* R0=SYS_pthread_start:  This a user pthread start
       *
       *   void up_pthread_start(pthread_startroutine_t entrypt,
       *                         pthread_addr_t arg) noreturn_function;
       *
       * At this point, the following values are saved in context:
       *
       *   R0 = SYS_pthread_start
       *   R1 = startup (trampoline)
       *   R2 = entrypt
       *   R3 = arg
       */

#if !defined(CONFIG_BUILD_FLAT) && !defined(CONFIG_DISABLE_PTHREAD)
      case SYS_pthread_start:
        {
          /* Set up to return to the user-space pthread start-up function in
           * unprivileged mode.
           */

          regs[REG_PC]         = (uint32_t)regs[REG_R1] & ~1;  /* startup */
          regs[REG_EXC_RETURN] = EXC_RETURN_THREAD;

          /* Return unprivileged mode */

          regs[REG_CONTROL]    =
            arm_control_set_mode(regs[REG_CONTROL], regs[REG_EXC_RETURN],
                                 true);

          /* Change the parameter ordering to match the expectation of the
           * user space pthread_startup:
           */

          regs[REG_R0]         = regs[REG_R2]; /* pthread entry */
          regs[REG_R1]         = regs[REG_R3]; /* arg */
        }
        break;
#endif

      /* R0=SYS_signal_handler:  This a user signal handler callback
       *
       * void signal_handler(_sa_sigaction_t sighand, int signo,
       *                     siginfo_t *info, void *ucontext);
       *
       * At this point, the following values are saved in context:
       *
       *   R0 = SYS_signal_handler
       *   R1 = sighand
       *   R2 = signo
       *   R3 = info
       *   R4 = ucontext
       */

#ifdef CONFIG_BUILD_PROTECTED
      case SYS_signal_handler:
        {
          struct tcb_s *rtcb   = this_task();

          /* Remember the caller's return address */

          DEBUGASSERT(rtcb->xcp.sigreturn == 0);
          rtcb->xcp.sigreturn  = regs[REG_PC];

          /* Set up to return to the user-space trampoline function in
           * unprivileged mode.
           */

          regs[REG_PC]         = (uint32_t)USERSPACE->signal_handler & ~1;
          regs[REG_EXC_RETURN] = EXC_RETURN_THREAD;

          /* Return unprivileged mode */

          regs[REG_CONTROL]    =
            arm_control_set_mode(regs[REG_CONTROL], regs[REG_EXC_RETURN],
                                 true);

          /* Change the parameter ordering to match the expectation of struct
           * userpace_s signal_handler.
           */

          regs[REG_R0]         = regs[REG_R1]; /* sighand */
          regs[REG_R1]         = regs[REG_R2]; /* signal */
          regs[REG_R2]         = regs[REG_R3]; /* info */
          regs[REG_R3]         = regs[REG_R4]; /* ucontext */
        }
        break;
#endif

      /* R0=SYS_signal_handler_return:  This a user signal handler callback
       *
       *   void signal_handler_return(void);
       *
       * At this point, the following values are saved in context:
       *
       *   R0 = SYS_signal_handler_return
       */

#ifdef CONFIG_BUILD_PROTECTED
      case SYS_signal_handler_return:
        {
          struct tcb_s *rtcb   = this_task();

          /* Set up to return to the kernel-mode signal dispatching logic. */

          DEBUGASSERT(rtcb->xcp.sigreturn != 0);

          regs[REG_PC]         = rtcb->xcp.sigreturn & ~1;
          regs[REG_EXC_RETURN] = EXC_RETURN_THREAD;

          /* Return privileged mode */

          regs[REG_CONTROL]    =
            arm_control_set_mode(regs[REG_CONTROL], regs[REG_EXC_RETURN],
                                 false);
          rtcb->xcp.sigreturn  = 0;
        }
        break;
#endif

      /* This is not an architecture-specific system call.  If NuttX is built
       * as a standalone kernel with a system call interface, then all of the
       * additional system calls must be handled as in the default case.
       */

      default:
        {
#ifdef CONFIG_LIB_SYSCALL
          struct tcb_s *rtcb = this_task();
          int index = rtcb->xcp.nsyscalls;
          uint32_t dispatch_excreturn = EXC_RETURN_THREAD;

          /* Verify that the SYS call number is within range */

          DEBUGASSERT(cmd >= CONFIG_SYS_RESERVED && cmd < SYS_maxsyscall);

          /* Make sure that there is a no saved syscall return address.  We
           * cannot yet handle nested system calls.
           */

          DEBUGASSERT(index < CONFIG_SYS_NNEST);

          /* Use ip to create a debug frame.
           * we can use gdb backtrace from syscall to user space.
           */

          regs[REG_IP] = regs[REG_PC];

          /* Setup to return to arm_dispatch_syscall in privileged mode. */

          rtcb->xcp.syscall[index].sysreturn  = regs[REG_PC];
          rtcb->xcp.syscall[index].excreturn  = regs[REG_EXC_RETURN];
          rtcb->xcp.syscall[index].ctrlreturn = regs[REG_CONTROL];
          rtcb->xcp.nsyscalls  = index + 1;

#  ifdef CONFIG_ARMV8M_LAZYFPU
          /* Keep the syscall dispatcher's exception frame shape compatible
           * with the saved user frame.  SYS_syscall_return restores the saved
           * EXC_RETURN into the current SVC frame; changing between basic and
           * extended FP frames here would make exception return decode the
           * wrong stack layout.
           */

          if ((regs[REG_EXC_RETURN] & EXC_RETURN_STD_CONTEXT) == 0)
            {
              dispatch_excreturn &= ~EXC_RETURN_STD_CONTEXT;
            }
#  endif

#  ifdef CONFIG_ARMV8M_SYSCALL_KERNEL_STACK
          if (index == 0 &&
              (regs[REG_EXC_RETURN] & EXC_RETURN_PROCESS_STACK) != 0)
            {
              uint32_t kstack_excreturn = dispatch_excreturn;
              FAR uint32_t *kregs;

              /* Run only the outermost user syscall on the internal stack.
               * Nested syscalls are already in privileged kernel context and
               * should keep using the current kernel-side frame.
               */

#    ifdef CONFIG_ARMV8M_SYSCALL_KERNEL_STACK_BASIC_FRAME
              /* Diagnostic/optimization mode: the dispatcher itself does not
               * need the caller's volatile FP frame, so use a basic frame on
               * the syscall stack while preserving the original user frame for
               * SYS_syscall_return.
               */

              kstack_excreturn |= EXC_RETURN_STD_CONTEXT;
#    endif

              kregs = arm_syscall_kstack_frame(rtcb, regs,
                                                kstack_excreturn);

              if (kregs != regs)
                {
                  regs = kregs;
#    ifdef CONFIG_ARMV8M_SYSCALL_KERNEL_STACK_PSP
                  /* Keep the dispatcher in thread mode on PSP, but point PSP
                   * at the internal syscall stack.  MSP remains the exception
                   * stack, which is important when the syscall stack is later
                   * deferred for release.
                   */

                  dispatch_excreturn = kstack_excreturn;
#    else
                  dispatch_excreturn =
                    kstack_excreturn & ~EXC_RETURN_PROCESS_STACK;
#    endif
                }
            }
#  endif

          regs[REG_EXC_RETURN] = dispatch_excreturn;
          regs[REG_PC]         = (uint32_t)arm_dispatch_syscall & ~1;

          /* Return privileged mode */

          regs[REG_CONTROL]    =
            arm_control_set_mode(regs[REG_CONTROL], regs[REG_EXC_RETURN],
                                 false);
#  ifdef CONFIG_ARMV8M_SYSCALL_DISPATCH_BASEPRI0
          /* The dispatcher and serial/semaphore wake paths rely on ordinary
           * maskable interrupts.  Do not inherit a user BASEPRI value into
           * privileged syscall execution.
           */

          regs[REG_BASEPRI]    = 0;
#  endif

          /* Offset R0 to account for the reserved values */

          regs[REG_R0]        -= CONFIG_SYS_RESERVED;

          /* Indicate that we are in a syscall handler. */

          rtcb->flags         |= TCB_FLAG_SYSCALL;
#else
          svcerr("ERROR: Bad SYS call: %" PRId32 "\n", regs[REG_R0]);
#endif
        }
        break;
    }

#ifdef CONFIG_ARMV8M_SYSCALL_BARRIER
  /* Optional bring-up fence for external cacheable SVC/user frames.  Normal
   * builds can leave this off once the frame-shape and BASEPRI handoff are
   * correct.
   */

  UP_DSB();
  UP_ISB();
#endif

  /* Report what happened.  That might difficult in the case of a context
   * switch.
   */

#ifdef CONFIG_DEBUG_SYSCALL_INFO
#  ifndef CONFIG_DEBUG_SVCALL
  if (cmd > SYS_switch_context)
#  endif
    {
      svcinfo("SVCall Return:\n");
      svcinfo("  R0: %08x %08x %08x %08x %08x %08x %08x %08x\n",
              regs[REG_R0],  regs[REG_R1], regs[REG_R2],  regs[REG_R3],
              regs[REG_R4],  regs[REG_R5], regs[REG_R6],  regs[REG_R7]);
      svcinfo("  R8: %08x %08x %08x %08x %08x %08x %08x %08x\n",
              regs[REG_R8],  regs[REG_R9], regs[REG_R10], regs[REG_R11],
              regs[REG_R12], regs[REG_R13], regs[REG_R14], regs[REG_R15]);
      svcinfo(" PSR: %08x EXC_RETURN: %08x CONTROL: %08x\n",
              regs[REG_XPSR], regs[REG_EXC_RETURN], regs[REG_CONTROL]);
    }
#endif

  UNUSED(tcb);
  return OK;
}
