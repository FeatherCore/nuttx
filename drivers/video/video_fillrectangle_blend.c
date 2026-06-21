/****************************************************************************
 * graphics/nxglib/fb/nxglib_fillrectangle_blend.c
 *
 * Alpha-blended fill rectangle for framebuffer.
 * Takes RGBA8888 color, reads current pixel, blends, writes back.
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/video/fb.h>
#include <nuttx/nx/nxglib.h>

#include <stdint.h>

static uint8_t nxgl_blend8(uint8_t s, uint8_t d, uint8_t a)
{
  return (uint8_t)(((uint16_t)s * a + (uint16_t)d * (255 - a) + 127) / 255);
}

static int nxgl_blend_clip(FAR struct fb_planeinfo_s *pinfo,
                            int x, int y, int bpp)
{
  FAR uint8_t *addr;
  if (x < 0 || y < 0 || pinfo == NULL || pinfo->fbmem == NULL)
    return 0;
  if (pinfo->fblen == 0) return 1; /* can't check, assume ok */
  addr = (FAR uint8_t *)pinfo->fbmem +
         (unsigned)y * pinfo->stride + (unsigned)x * (bpp / 8);
  if (addr < (FAR uint8_t *)pinfo->fbmem) return 0;
  if (addr + (bpp / 8) > (FAR uint8_t *)pinfo->fbmem + pinfo->fblen)
    return 0;
  return 1;
}

void nxgl_fillrectangle_blend(FAR struct fb_planeinfo_s *pinfo,
                               FAR const struct nxgl_rect_s *rect,
                               uint32_t rgba, int bpp)
{
  FAR uint8_t *line;
  int width, rows, x, y;
  int bytes;
  uint8_t sa, sr, sg, sb;

  sa = (uint8_t)rgba;
  if (sa == 0) return;
  sr = (uint8_t)(rgba >> 24);
  sg = (uint8_t)(rgba >> 16);
  sb = (uint8_t)(rgba >> 8);

  width = rect->pt2.x - rect->pt1.x + 1;
  rows  = rect->pt2.y - rect->pt1.y + 1;
  bytes = bpp / 8;

  /* Clip to framebuffer bounds */
  if (rect->pt1.x < 0 || rect->pt1.y < 0) return;

  line = (FAR uint8_t *)pinfo->fbmem +
         (unsigned)rect->pt1.y * pinfo->stride +
         (unsigned)rect->pt1.x * bytes;

  /* Check last pixel is in bounds */
  if (!nxgl_blend_clip(pinfo, rect->pt2.x, rect->pt2.y, bpp))
    return;

  if (sa == 255)
    {
      /* Opaque: write without readback */
      for (y = 0; y < rows; y++)
        {
          FAR uint8_t *dst = line;
          switch (bpp)
            {
              case 16:
                {
                  uint16_t v = (uint16_t)(((uint16_t)(sr & 0xf8) << 8) |
                                ((uint16_t)(sg & 0xfc) << 3) |
                                (sb >> 3));
                  for (x = 0; x < width; x++)
                    { *(FAR uint16_t *)dst = v; dst += 2; }
                }
                break;
              case 24:
                for (x = 0; x < width; x++)
                  { dst[0] = sr; dst[1] = sg; dst[2] = sb; dst += 3; }
                break;
              case 32:
              default:
                {
                  uint32_t v = ((uint32_t)sa << 24) |
                               ((uint32_t)sr << 16) |
                               ((uint32_t)sg << 8) | sb;
                  for (x = 0; x < width; x++)
                    { *(FAR uint32_t *)dst = v; dst += 4; }
                }
                break;
            }
          line += pinfo->stride;
        }
    }
  else
    {
      /* Alpha blended: read, blend, write back */
      for (y = 0; y < rows; y++)
        {
          FAR uint8_t *dst = line;
          for (x = 0; x < width; x++)
            {
              uint32_t cur;
              uint8_t dr, dg, db;
              switch (bpp)
                {
                  case 16:
                    cur = *(FAR uint16_t *)dst;
                    dr = (uint8_t)((cur & 0xf800) >> 8);
                    dg = (uint8_t)((cur & 0x07e0) >> 3);
                    db = (uint8_t)((cur & 0x001f) << 3);
                    *(FAR uint16_t *)dst =
                      (uint16_t)(((uint16_t)(nxgl_blend8(sr, dr, sa) & 0xf8) << 8) |
                                  ((uint16_t)(nxgl_blend8(sg, dg, sa) & 0xfc) << 3) |
                                  (nxgl_blend8(sb, db, sa) >> 3));
                    dst += 2;
                    break;
                  case 24:
                    dr = dst[0]; dg = dst[1]; db = dst[2];
                    dst[0] = nxgl_blend8(sr, dr, sa);
                    dst[1] = nxgl_blend8(sg, dg, sa);
                    dst[2] = nxgl_blend8(sb, db, sa);
                    dst += 3;
                    break;
                  case 32:
                  default:
                    cur = *(FAR uint32_t *)dst;
                    dr = (uint8_t)(cur >> 16);
                    dg = (uint8_t)(cur >> 8);
                    db = (uint8_t)cur;
                    *(FAR uint32_t *)dst =
                      ((uint32_t)sa << 24) |
                      ((uint32_t)nxgl_blend8(sr, dr, sa) << 16) |
                      ((uint32_t)nxgl_blend8(sg, dg, sa) << 8) |
                      nxgl_blend8(sb, db, sa);
                    dst += 4;
                    break;
                }
            }
          line += pinfo->stride;
        }
    }
}

/* Alpha-blended fill trapezoid */

void nxgl_filltrapezoid_blend(FAR struct fb_planeinfo_s *pinfo,
                               FAR const struct nxgl_trapezoid_s *trap,
                               FAR const struct nxgl_rect_s *bounds,
                               uint32_t rgba, int bpp)
{
  int y, t, b, x;
  int32_t lx1, rx1, lx2, rx2, left, right, l, r;

  t = trap->top.y; b = trap->bot.y;
  if (t > b) { int tmp = t; t = b; b = tmp; }
  if (t < bounds->pt1.y) t = bounds->pt1.y;
  if (b > bounds->pt2.y) b = bounds->pt2.y;
  if (t >= b) return;

  lx1 = (int32_t)(trap->top.x1 >> 16);
  rx1 = (int32_t)(trap->top.x2 >> 16);
  lx2 = (int32_t)(trap->bot.x1 >> 16);
  rx2 = (int32_t)(trap->bot.x2 >> 16);
  if (lx1 > rx1) { int32_t tmp = lx1; lx1 = rx1; rx1 = tmp; }
  if (lx2 > rx2) { int32_t tmp = lx2; lx2 = rx2; rx2 = tmp; }

  for (y = t; y <= b; y++)
    {
      if (y == b)      { left = lx2; right = rx2; }
      else if (y == t) { left = lx1; right = rx1; }
      else
        {
          int span = b - t; if (span <= 0) span = 1;
          l = lx1 + ((lx2 - lx1) * (y - t)) / span;
          r = rx1 + ((rx2 - rx1) * (y - t)) / span;
          if (l > r) { int32_t tmp = l; l = r; r = tmp; }
          left = l; right = r;
        }
      if (left < bounds->pt1.x) left = bounds->pt1.x;
      if (right > bounds->pt2.x) right = bounds->pt2.x;
      if (left > right) continue;

      {
        struct nxgl_rect_s line;
        line.pt1.x = (nxgl_coord_t)left; line.pt1.y = (nxgl_coord_t)y;
        line.pt2.x = (nxgl_coord_t)right; line.pt2.y = (nxgl_coord_t)y;
        nxgl_fillrectangle_blend(pinfo, &line, rgba, bpp);
      }
    }
}

/* Alpha-blended convex polygon via per-scanline edge walk */

void nxgl_fillpolygon_blend(FAR struct fb_planeinfo_s *pinfo,
                             FAR const struct nxgl_point_s *verts,
                             int nverts, uint32_t rgba, int bpp)
{
  int i, y, x;
  int32_t min_y, max_y, xs[8];
  int count;
  int32_t left, right;

  if (nverts < 3 || nverts > 8) return;
  min_y = verts[0].y; max_y = verts[0].y;
  for (i = 1; i < nverts; i++)
    { if (verts[i].y < min_y) min_y = verts[i].y;
      if (verts[i].y > max_y) max_y = verts[i].y; }

  for (y = min_y; y <= max_y; y++)
    {
      count = 0;
      for (i = 0; i < nverts; i++)
        {
          int j = (i + 1) % nverts;
          if (verts[i].y == verts[j].y) continue;
          if ((y < verts[i].y && y < verts[j].y) ||
              (y >= verts[i].y && y >= verts[j].y)) continue;
          xs[count] = verts[i].x +
            ((verts[j].x - verts[i].x) * (y - verts[i].y)) /
            (verts[j].y - verts[i].y);
          count++; if (count >= 8) break;
        }
      if (count < 2) continue;
      left = xs[0]; right = xs[0];
      for (i = 1; i < count; i++)
        { if (xs[i] < left) left = xs[i];
          if (xs[i] > right) right = xs[i]; }
      if (right < left) { int32_t t = left; left = right; right = t; }
      {
        struct nxgl_rect_s line;
        line.pt1.x = (nxgl_coord_t)left; line.pt1.y = (nxgl_coord_t)y;
        line.pt2.x = (nxgl_coord_t)right; line.pt2.y = (nxgl_coord_t)y;
        nxgl_fillrectangle_blend(pinfo, &line, rgba, bpp);
      }
    }
}

/* Thick line with alpha (Bresenham) */

void nxgl_drawline_blend(FAR struct fb_planeinfo_s *pinfo,
                          FAR const struct nxgl_vector_s *vec,
                          nxgl_coord_t width, uint32_t rgba, int bpp)
{
  int32_t x, y, dx, dy, sx, sy, err, e2, half, px, py;

  if (width < 1) width = 1;
  half = width / 2;
  x = vec->pt1.x; y = vec->pt1.y;
  dx = (int32_t)(vec->pt2.x > x ? vec->pt2.x - x : x - vec->pt2.x);
  dy = -(int32_t)(vec->pt2.y > y ? vec->pt2.y - y : y - vec->pt2.y);
  sx = x < vec->pt2.x ? 1 : -1;
  sy = y < vec->pt2.y ? 1 : -1;
  err = dx + dy;

  while (1)
    {
      for (py = y - half; py <= y + half; py++)
        for (px = x - half; px <= x + half; px++)
          {
            struct nxgl_rect_s dot;
            dot.pt1.x = (nxgl_coord_t)px; dot.pt1.y = (nxgl_coord_t)py;
            dot.pt2.x = (nxgl_coord_t)px; dot.pt2.y = (nxgl_coord_t)py;
            nxgl_fillrectangle_blend(pinfo, &dot, rgba, bpp);
          }
      if (x == vec->pt2.x && y == vec->pt2.y) break;
      e2 = err * 2;
      if (e2 >= dy) { err += dy; x += sx; }
      if (e2 <= dx) { err += dx; y += sy; }
    }
}

/* Scaled nearest-neighbor blit from RGBA8888 source with global alpha */

void nxgl_blit_scale(FAR struct fb_planeinfo_s *pinfo,
                      FAR const struct nxgl_rect_s *dest,
                      FAR const void *src_rgba,
                      int src_w, int src_h, int src_stride,
                      int bpp, uint8_t global_alpha)
{
  FAR const uint32_t *sp = (FAR const uint32_t *)src_rgba;
  int dw, dh, dx, dy, sx, sy;

  dw = dest->pt2.x - dest->pt1.x + 1;
  dh = dest->pt2.y - dest->pt1.y + 1;
  if (dw <= 0 || dh <= 0) return;

  for (dy = 0; dy < dh; dy++)
    {
      int dst_y = dest->pt1.y + dy;
      sy = (dy * src_h) / dh;
      if (sy >= src_h) sy = src_h - 1;
      for (dx = 0; dx < dw; dx++)
        {
          int dst_x = dest->pt1.x + dx;
          sx = (dx * src_w) / dw;
          if (sx >= src_w) sx = src_w - 1;
          {
            uint32_t p = sp[(unsigned)sy * src_stride + sx];
            uint8_t a = (uint8_t)p;
            a = (uint8_t)(((uint16_t)a * global_alpha + 127) / 255);
            if (a > 0)
              {
                uint32_t rgba = ((uint32_t)(uint8_t)(p >> 24) << 24) |
                                ((uint32_t)(uint8_t)(p >> 16) << 16) |
                                ((uint32_t)(uint8_t)(p >> 8) << 8) | a;
                struct nxgl_rect_s dot;
                dot.pt1.x = (nxgl_coord_t)dst_x;
                dot.pt1.y = (nxgl_coord_t)dst_y;
                dot.pt2.x = (nxgl_coord_t)dst_x;
                dot.pt2.y = (nxgl_coord_t)dst_y;
                nxgl_fillrectangle_blend(pinfo, &dot, rgba, bpp);
              }
          }
        }
    }
}

/* Quad blit from RGBA8888 source with alpha (per-scanline edge walk) */

void nxgl_blit_quad(FAR struct fb_planeinfo_s *pinfo,
                     FAR const void *src_rgba,
                     int src_w, int src_h, int src_stride,
                     FAR const struct nxgl_point_s quad[4],
                     int bpp, uint8_t global_alpha)
{
  FAR const uint32_t *sp = (FAR const uint32_t *)src_rgba;
  int32_t min_y, max_y, xs[8];
  int i, y, x, count, sx, sy;
  int32_t y_span, x_span, left, right;

  min_y = quad[0].y; max_y = quad[0].y;
  for (i = 1; i < 4; i++)
    { if (quad[i].y < min_y) min_y = quad[i].y;
      if (quad[i].y > max_y) max_y = quad[i].y; }
  y_span = max_y - min_y; if (y_span <= 0) y_span = 1;

  for (y = min_y; y <= max_y; y++)
    {
      count = 0;
      for (i = 0; i < 4; i++)
        {
          int j = (i + 1) & 3;
          if (quad[i].y == quad[j].y) continue;
          if ((y < quad[i].y && y < quad[j].y) ||
              (y >= quad[i].y && y >= quad[j].y)) continue;
          xs[count] = quad[i].x +
            ((quad[j].x - quad[i].x) * (y - quad[i].y)) /
            (quad[j].y - quad[i].y);
          count++; if (count >= 8) break;
        }
      if (count < 2) continue;
      left = xs[0]; right = xs[0];
      for (i = 1; i < count; i++)
        { if (xs[i] < left) left = xs[i];
          if (xs[i] > right) right = xs[i]; }
      if (right < left) { int32_t t = left; left = right; right = t; }
      x_span = right - left; if (x_span <= 0) x_span = 1;
      sy = ((y - min_y) * (src_h - 1)) / y_span;
      if (sy >= src_h) sy = src_h - 1;
      for (x = left; x <= right; x++)
        {
          sx = ((x - left) * (src_w - 1)) / x_span;
          if (sx >= src_w) sx = src_w - 1;
          {
            uint32_t p = sp[(unsigned)sy * src_stride + sx];
            uint8_t a = (uint8_t)p;
            a = (uint8_t)(((uint16_t)a * global_alpha + 127) / 255);
            if (a > 0)
              {
                uint32_t rgba = ((uint32_t)(uint8_t)(p >> 24) << 24) |
                                ((uint32_t)(uint8_t)(p >> 16) << 16) |
                                ((uint32_t)(uint8_t)(p >> 8) << 8) | a;
                struct nxgl_rect_s dot;
                dot.pt1.x = (nxgl_coord_t)x; dot.pt1.y = (nxgl_coord_t)y;
                dot.pt2.x = (nxgl_coord_t)x; dot.pt2.y = (nxgl_coord_t)y;
                nxgl_fillrectangle_blend(pinfo, &dot, rgba, bpp);
              }
          }
        }
    }
}
