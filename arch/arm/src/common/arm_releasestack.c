/****************************************************************************
 * arch/arm/src/common/arm_releasestack.c
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

#include <sched.h>
#include <nuttx/debug.h>

#include <nuttx/arch.h>
#include <nuttx/irq.h>
#include <nuttx/kmalloc.h>

#include "arm_internal.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#if defined(CONFIG_LIB_SYSCALL) && defined(CONFIG_ARMV8M_SYSCALL_KERNEL_STACK)
static FAR void *g_armv8m_pending_kstacks;

void armv8m_syscall_kstack_defer(FAR void *kstack)
{
  irqstate_t flags;

  if (kstack == NULL)
    {
      return;
    }

  flags = up_irq_save();

  /* Reuse the first word of the stack block as a tiny pending-free list node.
   * The memory is no longer a live stack once it reaches this path.
   */

  *(FAR void **)kstack = g_armv8m_pending_kstacks;
  g_armv8m_pending_kstacks = kstack;
  up_irq_restore(flags);
}

void armv8m_syscall_kstack_drain(void)
{
  FAR void *kstack;
  FAR void *next;
  irqstate_t flags;

  flags = up_irq_save();
  kstack = g_armv8m_pending_kstacks;
  g_armv8m_pending_kstacks = NULL;
  up_irq_restore(flags);

  /* Drain from an SVC entry that is known to run on the exception stack.  That
   * avoids freeing a per-task syscall stack while it is still the active PSP.
   */

  while (kstack != NULL)
    {
      next = *(FAR void **)kstack;
      kmm_free(kstack);
      kstack = next;
    }
}
#endif

/****************************************************************************
 * Name: up_release_stack
 *
 * Description:
 *   A task has been stopped. Free all stack related resources retained in
 *   the defunct TCB.
 *
 * Input Parameters:
 *   - dtcb:  The TCB containing information about the stack to be released
 *   - ttype:  The thread type.  This may be one of following (defined in
 *     include/nuttx/sched.h):
 *
 *       TCB_FLAG_TTYPE_TASK     Normal user task
 *       TCB_FLAG_TTYPE_PTHREAD  User pthread
 *       TCB_FLAG_TTYPE_KERNEL   Kernel thread
 *
 *     This thread type is normally available in the flags field of the TCB,
 *     however, there are certain error recovery contexts where the TCB may
 *     not be fully initialized when up_release_stack is called.
 *
 *     If either CONFIG_BUILD_PROTECTED or CONFIG_BUILD_KERNEL are defined,
 *     then this thread type may affect how the stack is freed.  For example,
 *     kernel thread stacks may have been allocated from protected kernel
 *     memory.  Stacks for user tasks and threads must have come from memory
 *     that is accessible to user code.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void up_release_stack(struct tcb_s *dtcb, uint8_t ttype)
{
#if defined(CONFIG_LIB_SYSCALL) && defined(CONFIG_ARMV8M_SYSCALL_KERNEL_STACK)
  if (dtcb->xcp.kstack)
    {
      /* A protected task may exit while still running on this per-task
       * syscall stack.  Defer the actual free until a later SVC entry that
       * is running from the exception stack, not from the defunct task stack.
       */

      armv8m_syscall_kstack_defer(dtcb->xcp.kstack);
      dtcb->xcp.kstack = NULL;
      dtcb->xcp.ustkptr = NULL;
    }
#endif

  /* Is there a stack allocated? */

  if (dtcb->stack_alloc_ptr && (dtcb->flags & TCB_FLAG_FREE_STACK))
    {
#ifdef CONFIG_MM_KERNEL_HEAP
      /* Use the kernel allocator if this is a kernel thread */

      if (ttype == TCB_FLAG_TTYPE_KERNEL)
        {
          kmm_free(dtcb->stack_alloc_ptr);
        }
      else
#endif
        {
          /* Use the user-space allocator if this is a task or pthread */

          kumm_free(dtcb->stack_alloc_ptr);
        }
    }

  /* Mark the stack freed */

  dtcb->flags &= ~TCB_FLAG_FREE_STACK;
  dtcb->stack_alloc_ptr = NULL;
  dtcb->stack_base_ptr = NULL;
  dtcb->adj_stack_size = 0;
}
