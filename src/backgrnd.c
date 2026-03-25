/* 
 * OpenTyrian: A modern cross-platform port of Tyrian
 * Copyright (C) 2007-2009  The OpenTyrian Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "backgrnd.h"

#include "config.h"
#include "mtrand.h"
#include "opentyr.h"
#include "varz.h"
#include "video.h"

#include <assert.h>

/*Special Background 2 and Background 3*/

/*Back Pos 3*/
JE_word backPos, backPos2, backPos3;
JE_word backMove, backMove2, backMove3;

/*Main Maps*/
JE_word mapX, mapY, mapX2, mapX3, mapY2, mapY3;
JE_byte **mapYPos, **mapY2Pos, **mapY3Pos;
JE_word mapXPos, oldMapXOfs, mapXOfs, mapX2Ofs, mapX2Pos, mapX3Pos, oldMapX3Ofs, mapX3Ofs, tempMapXOfs;
intptr_t mapXbpPos, mapX2bpPos, mapX3bpPos;
JE_byte map1YDelay, map1YDelayMax, map2YDelay, map2YDelayMax;

JE_boolean  anySmoothies;
JE_byte     smoothie_data[9]; /* [1..9] */

void JE_darkenBackground(JE_word neat)  /* wild detail level */
{
	Uint8 *s = VGAScreen->pixels; /* screen pointer, 8-bit specific */
	int x, y;
	
	s += 24;
	
	for (y = 184; y; y--)
	{
		for (x = 264; x; x--)
		{
			*s = ((((*s & 0x0f) << 4) - (*s & 0x0f) + ((((x - neat - y) >> 2) + *(s-2) + (y == 184 ? 0 : *(s-(VGAScreen->pitch-1)))) & 0x0f)) >> 4) | (*s & 0xf0);
			s++;
		}
		s += VGAScreen->pitch - 264;
	}
}

__attribute__((section(".fasttext")))
void blit_background_row(SDL_Surface *surface, int x, int y, Uint8 **map)
{
	assert(surface->format->BitsPerPixel == 8);
	
	Uint8 *pixels = (Uint8 *)surface->pixels + (y * surface->pitch) + x,
	      *pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	      *pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	for (int y = 0; y < 28; y++)
	{
		// not drawing on screen yet; skip y
		if ((pixels + (12 * 24)) < pixels_ll)
		{
			pixels += surface->pitch;
			continue;
		}
		
		for (int tile = 0; tile < 12; tile++)
		{
			Uint8 *data = *(map + tile);
			
			// no tile; skip tile
			if (data == NULL)
			{
				pixels += 24;
				continue;
			}
			
			data += y * 24;
			
			for (int x = 24; x; x--)
			{
				if (pixels >= pixels_ul)
					return;
				if (pixels >= pixels_ll && *data != 0)
					*pixels = *data;
				
				pixels++;
				data++;
			}
		}
		
		pixels += surface->pitch - 12 * 24;
	}
}

__attribute__((section(".fasttext")))
void blit_background_row_blend(SDL_Surface *surface, int x, int y, Uint8 **map)
{
	assert(surface->format->BitsPerPixel == 8);
	
	Uint8 *pixels = (Uint8 *)surface->pixels + (y * surface->pitch) + x,
	      *pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	      *pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	for (int y = 0; y < 28; y++)
	{
		// not drawing on screen yet; skip y
		if ((pixels + (12 * 24)) < pixels_ll)
		{
			pixels += surface->pitch;
			continue;
		}
		
		for (int tile = 0; tile < 12; tile++)
		{
			Uint8 *data = *(map + tile);
			
			// no tile; skip tile
			if (data == NULL)
			{
				pixels += 24;
				continue;
			}
			
			data += y * 24;
			
			for (int x = 24; x; x--)
			{
				if (pixels >= pixels_ul)
					return;
				if (pixels >= pixels_ll && *data != 0)
					*pixels = (*data & 0xf0) | (((*pixels & 0x0f) + (*data & 0x0f)) / 2);
				
				pixels++;
				data++;
			}
		}
		
		pixels += surface->pitch - 12 * 24;
	}
}

void draw_background_1(SDL_Surface *surface)
{
	SDL_FillRect(surface, NULL, 0);
	
	Uint8 **map = (Uint8 **)mapYPos + mapXbpPos - 12;
	
	for (int i = -1; i < 7; i++)
	{
		blit_background_row(surface, mapXPos, (i * 28) + backPos, map);
		
		map += 14;
	}
}

void draw_background_2(SDL_Surface *surface)
{
	if (map2YDelayMax > 1 && backMove2 < 2)
		backMove2 = (map2YDelay == 1) ? 1 : 0;
	
	if (background2 != 0)
	{
		// water effect combines background 1 and 2 by synchronizing the x coordinate
		int x = smoothies[1] ? mapXPos : mapX2Pos;
		
		Uint8 **map = (Uint8 **)mapY2Pos + (smoothies[1] ? mapXbpPos : mapX2bpPos) - 12;
		
		for (int i = -1; i < 7; i++)
		{
			blit_background_row(surface, x, (i * 28) + backPos2, map);
			
			map += 14;
		}
	}
	
	/*Set Movement of background*/
	if (--map2YDelay == 0)
	{
		map2YDelay = map2YDelayMax;
		
		backPos2 += backMove2;
		
		if (backPos2 >  27)
		{
			backPos2 -= 28;
			mapY2--;
			mapY2Pos -= 14;  /*Map Width*/
		}
	}
}

void draw_background_2_blend(SDL_Surface *surface)
{
	if (map2YDelayMax > 1 && backMove2 < 2)
		backMove2 = (map2YDelay == 1) ? 1 : 0;
	
	Uint8 **map = (Uint8 **)mapY2Pos + mapX2bpPos - 12;
	
	for (int i = -1; i < 7; i++)
	{
		blit_background_row_blend(surface, mapX2Pos, (i * 28) + backPos2, map);
		
		map += 14;
	}
	
	/*Set Movement of background*/
	if (--map2YDelay == 0)
	{
		map2YDelay = map2YDelayMax;
		
		backPos2 += backMove2;
		
		if (backPos2 >  27)
		{
			backPos2 -= 28;
			mapY2--;
			mapY2Pos -= 14;  /*Map Width*/
		}
	}
}

void draw_background_3(SDL_Surface *surface)
{
	/* Movement of background */
	backPos3 += backMove3;
	
	if (backPos3 > 27)
	{
		backPos3 -= 28;
		mapY3--;
		mapY3Pos -= 15;   /*Map Width*/
	}
	
	Uint8 **map = (Uint8 **)mapY3Pos + mapX3bpPos - 12;
	
	for (int i = -1; i < 7; i++)
	{
		blit_background_row(surface, mapX3Pos, (i * 28) + backPos3, map);
		
		map += 15;
	}
}

__attribute__((section(".fasttext")))
void JE_filterScreen(JE_shortint col, JE_shortint int_)
{
	Uint8 *s = NULL; /* screen pointer, 8-bit specific */
	int x, y;
	unsigned int temp;
	
	if (filterFade)
	{
		levelBrightness += levelBrightnessChg;
		if ((filterFadeStart && levelBrightness < -14) || levelBrightness > 14)
		{
			levelBrightnessChg = -levelBrightnessChg;
			filterFadeStart = false;
			levelFilter = levelFilterNew;
		}
		if (!filterFadeStart && levelBrightness == 0)
		{
			filterFade = false;
			levelBrightness = -99;
		}
	}
	
	if (col != -99 && filtrationAvail)
	{
		s = VGAScreen->pixels;
		s += 24;
		
		col <<= 4;
		
		for (y = 184; y; y--)
		{
			for (x = 264; x; x--)
			{
				*s = col | (*s & 0x0f);
				s++;
			}
			s += VGAScreen->pitch - 264;
		}
	}
	
	if (int_ != -99 && explosionTransparent)
	{
		s = VGAScreen->pixels;
		s += 24;
		
		for (y = 184; y; y--)
		{
			for (x = 264; x; x--)
			{
				temp = (*s & 0x0f) + int_;
				*s = (*s & 0xf0) | (temp >= 0x1f ? 0 : (temp >= 0x0f ? 0x0f : temp));
				s++;
			}
			s += VGAScreen->pitch - 264;
		}
	}
}

void JE_checkSmoothies(void)
{
	anySmoothies = (processorType > 2 && processorType != 5 && (smoothies[1-1] || smoothies[2-1])) || 
	               (processorType > 1 && (smoothies[3-1] || smoothies[4-1] || smoothies[5-1]));
}

__attribute__((section(".fasttext")))
static void fast_lava_filter(SDL_Surface *dst, SDL_Surface *src)
{
	uint32_t *d32 = (uint32_t *)dst->pixels;
	const uint32_t *s32 = (const uint32_t *)src->pixels;
	int n = (320 * 185) / 4;
	
	while (n >= 4) {
		uint32_t s0 = s32[0], d0 = d32[0];
		uint32_t s1 = s32[1], d1 = d32[1];
		uint32_t s2 = s32[2], d2 = d32[2];
		uint32_t s3 = s32[3], d3 = d32[3];
		d32[0] = ((((s0 & 0x0F0F0F0F) + (d0 & 0x0F0F0F0F)) >> 1) & 0x0F0F0F0F) | 0x70707070;
		d32[1] = ((((s1 & 0x0F0F0F0F) + (d1 & 0x0F0F0F0F)) >> 1) & 0x0F0F0F0F) | 0x70707070;
		d32[2] = ((((s2 & 0x0F0F0F0F) + (d2 & 0x0F0F0F0F)) >> 1) & 0x0F0F0F0F) | 0x70707070;
		d32[3] = ((((s3 & 0x0F0F0F0F) + (d3 & 0x0F0F0F0F)) >> 1) & 0x0F0F0F0F) | 0x70707070;
		s32 += 4; d32 += 4; n -= 4;
	}
	while (n--) {
		uint32_t s = *s32++, d = *d32;
		*d32++ = ((((s & 0x0F0F0F0F) + (d & 0x0F0F0F0F)) >> 1) & 0x0F0F0F0F) | 0x70707070;
	}
}

static void original_lava_filter(SDL_Surface *dst, SDL_Surface *src)
{
	const int dst_pitch = dst->pitch;
	Uint8 *dst_pixel = (Uint8 *)dst->pixels + (185 * dst_pitch);
	const Uint8 * const dst_pixel_ll = (Uint8 *)dst->pixels;
	
	const int src_pitch = src->pitch;
	const Uint8 *src_pixel = (Uint8 *)src->pixels + (185 * src->pitch);
	const Uint8 * const src_pixel_ll = (Uint8 *)src->pixels;
	
	int w = 320 * 185 - 1;
	
	for (int y = 185 - 1; y >= 0; --y)
	{
		dst_pixel -= (dst_pitch - 320);
		src_pixel -= (src_pitch - 320);
		
		for (int x = 320 - 1; x >= 0; x -= 8)
		{
			int waver = abs(((w >> 9) & 0x0f) - 8) - 1;
			w -= 8;
			
			for (int xi = 8 - 1; xi >= 0; --xi)
			{
				--dst_pixel;
				--src_pixel;
				
				Uint8 value = 0;
				
				if (src_pixel + waver >= src_pixel_ll)
					value += (*(src_pixel + waver) & 0x0f) * 2;
				value += *(dst_pixel + waver + dst_pitch) & 0x0f;
				if (dst_pixel + waver - dst_pitch >= dst_pixel_ll)
					value += *(dst_pixel + waver - dst_pitch) & 0x0f;
				
				*dst_pixel = (value / 4) | 0x70;
			}
		}
	}
}

void lava_filter(SDL_Surface *dst, SDL_Surface *src)
{
	assert(src->format->BitsPerPixel == 8 && dst->format->BitsPerPixel == 8);
	original_lava_filter(dst, src);
}

__attribute__((section(".fasttext")))
static void fast_water_filter(SDL_Surface *dst, SDL_Surface *src)
{
	uint32_t hue = smoothie_data[1] << 4;
	uint32_t hue32 = hue | (hue << 8) | (hue << 16) | (hue << 24);
	
	uint32_t *d32 = (uint32_t *)dst->pixels;
	const uint32_t *s32 = (const uint32_t *)src->pixels;
	int n = (320 * 185) / 4;
	
	/* 100% Branchless 32-bit water filter. 
	 * Eliminates conditional jumps that flush the CPU pipeline. */
	while (n--) {
		uint32_t s = *s32++;
		
		/* Isolate the bits we care about: 0x30 in each byte */
		uint32_t check = s & 0x30303030;
		
		/* Shift down so the relevant bits are at the bottom of each byte */
		uint32_t c_or = ((check >> 4) | (check >> 5)) & 0x01010101;
		
		/* Multiply by 255 to expand 0x01 into 0xFF for the entire byte.
		 * mask will have 0xFF where the pixel was NOT blue, and 0x00 where it WAS blue. */
		uint32_t mask = c_or * 255; 
		
		/* Apply the new hue */
		uint32_t new_px = (s & 0x0F0F0F0F) | hue32;
		
		/* Blend using the mask: keep original if mask=0, use new if mask=0xFF */
		*d32++ = (s & ~mask) | (new_px & mask);
	}
}

static void original_water_filter(SDL_Surface *dst, SDL_Surface *src)
{
	Uint8 hue = smoothie_data[1] << 4;
	const int dst_pitch = dst->pitch;
	Uint8 *dst_pixel = (Uint8 *)dst->pixels + (185 * dst_pitch);
	const Uint8 *src_pixel = (Uint8 *)src->pixels + (185 * src->pitch);
	int w = 320 * 185 - 1;
	
	for (int y = 185 - 1; y >= 0; --y)
	{
		dst_pixel -= (dst_pitch - 320);
		src_pixel -= (src->pitch - 320);
		
		for (int x = 320 - 1; x >= 0; x -= 8)
		{
			int waver = abs(((w >> 10) & 0x07) - 4) - 1;
			w -= 8;
			
			for (int xi = 8 - 1; xi >= 0; --xi)
			{
				--dst_pixel;
				--src_pixel;
				
				if ((*src_pixel & 0x30) == 0)
				{
					*dst_pixel = *src_pixel;
				}
				else
				{
					Uint8 value = *src_pixel & 0x0f;
					value += *(dst_pixel + waver + dst_pitch) & 0x0f;
					*dst_pixel = (value / 2) | hue;
				}
			}
		}
	}
}

void water_filter(SDL_Surface *dst, SDL_Surface *src)
{
	assert(src->format->BitsPerPixel == 8 && dst->format->BitsPerPixel == 8);
	original_water_filter(dst, src);
}

__attribute__((section(".fasttext")))
static void fast_iced_blur_filter(SDL_Surface *dst, SDL_Surface *src)
{
	uint32_t *d32 = (uint32_t *)dst->pixels;
	const uint32_t *s32 = (const uint32_t *)src->pixels;
	int n = (320 * 184) / 4;
	
	while (n >= 4) {
		uint32_t s0 = s32[0], d0 = d32[0];
		uint32_t s1 = s32[1], d1 = d32[1];
		uint32_t s2 = s32[2], d2 = d32[2];
		uint32_t s3 = s32[3], d3 = d32[3];
		d32[0] = ((((s0 & 0x0F0F0F0F) + (d0 & 0x0F0F0F0F)) >> 1) & 0x0F0F0F0F) | 0x80808080;
		d32[1] = ((((s1 & 0x0F0F0F0F) + (d1 & 0x0F0F0F0F)) >> 1) & 0x0F0F0F0F) | 0x80808080;
		d32[2] = ((((s2 & 0x0F0F0F0F) + (d2 & 0x0F0F0F0F)) >> 1) & 0x0F0F0F0F) | 0x80808080;
		d32[3] = ((((s3 & 0x0F0F0F0F) + (d3 & 0x0F0F0F0F)) >> 1) & 0x0F0F0F0F) | 0x80808080;
		s32 += 4; d32 += 4; n -= 4;
	}
	while (n--) {
		uint32_t s = *s32++, d = *d32;
		*d32++ = ((((s & 0x0F0F0F0F) + (d & 0x0F0F0F0F)) >> 1) & 0x0F0F0F0F) | 0x80808080;
	}
}

static void original_iced_blur_filter(SDL_Surface *dst, SDL_Surface *src)
{
	Uint8 *dst_pixel = dst->pixels;
	const Uint8 *src_pixel = src->pixels;
	
	for (int y = 0; y < 184; ++y)
	{
		for (int x = 0; x < 320; ++x)
		{
			const Uint8 value = (*src_pixel & 0x0f) + (*dst_pixel & 0x0f);
			*dst_pixel = (value / 2) | 0x80;
			
			++dst_pixel;
			++src_pixel;
		}
		
		dst_pixel += (dst->pitch - 320);
		src_pixel += (src->pitch - 320);
	}
}

void iced_blur_filter(SDL_Surface *dst, SDL_Surface *src)
{
	assert(src->format->BitsPerPixel == 8 && dst->format->BitsPerPixel == 8);
	if (processorType == 5)
		fast_iced_blur_filter(dst, src);
	else
		original_iced_blur_filter(dst, src);
}

__attribute__((section(".fasttext")))
static void fast_blur_filter(SDL_Surface *dst, SDL_Surface *src)
{
	uint32_t *d32 = (uint32_t *)dst->pixels;
	const uint32_t *s32 = (const uint32_t *)src->pixels;
	int n = (320 * 184) / 4;
	
	while (n >= 4) {
		uint32_t s0 = s32[0], d0 = d32[0];
		uint32_t s1 = s32[1], d1 = d32[1];
		uint32_t s2 = s32[2], d2 = d32[2];
		uint32_t s3 = s32[3], d3 = d32[3];
		d32[0] = ((((s0 & 0x0F0F0F0F) + (d0 & 0x0F0F0F0F)) >> 1) & 0x0F0F0F0F) | (s0 & 0xF0F0F0F0);
		d32[1] = ((((s1 & 0x0F0F0F0F) + (d1 & 0x0F0F0F0F)) >> 1) & 0x0F0F0F0F) | (s1 & 0xF0F0F0F0);
		d32[2] = ((((s2 & 0x0F0F0F0F) + (d2 & 0x0F0F0F0F)) >> 1) & 0x0F0F0F0F) | (s2 & 0xF0F0F0F0);
		d32[3] = ((((s3 & 0x0F0F0F0F) + (d3 & 0x0F0F0F0F)) >> 1) & 0x0F0F0F0F) | (s3 & 0xF0F0F0F0);
		s32 += 4; d32 += 4; n -= 4;
	}
	while (n--) {
		uint32_t s = *s32++, d = *d32;
		*d32++ = ((((s & 0x0F0F0F0F) + (d & 0x0F0F0F0F)) >> 1) & 0x0F0F0F0F) | (s & 0xF0F0F0F0);
	}
}

static void original_blur_filter(SDL_Surface *dst, SDL_Surface *src)
{
	Uint8 *dst_pixel = dst->pixels;
	const Uint8 *src_pixel = src->pixels;
	
	for (int y = 0; y < 184; ++y)
	{
		for (int x = 0; x < 320; ++x)
		{
			const Uint8 value = (*src_pixel & 0x0f) + (*dst_pixel & 0x0f);
			*dst_pixel = (value / 2) | (*src_pixel & 0xf0);
			
			++dst_pixel;
			++src_pixel;
		}
		
		dst_pixel += (dst->pitch - 320);
		src_pixel += (src->pitch - 320);
	}
}

void blur_filter(SDL_Surface *dst, SDL_Surface *src)
{
	assert(src->format->BitsPerPixel == 8 && dst->format->BitsPerPixel == 8);
	if (processorType == 5)
		fast_blur_filter(dst, src);
	else
		original_blur_filter(dst, src);
}

/* Background Starfield */
typedef struct
{
	Uint8 color;
	JE_word position; // relies on overflow wrap-around
	int speed;
} StarfieldStar;

#define MAX_STARS 100
#define STARFIELD_HUE 0x90
static StarfieldStar starfield_stars[MAX_STARS];
int starfield_speed;

void initialize_starfield(void)
{
	for (int i = MAX_STARS-1; i >= 0; --i)
	{
		starfield_stars[i].position = mt_rand() % 320 + mt_rand() % 200 * VGAScreen->pitch;
		starfield_stars[i].speed = mt_rand() % 3 + 2;
		starfield_stars[i].color = mt_rand() % 16 + STARFIELD_HUE;
	}
}

void update_and_draw_starfield(SDL_Surface* surface, int move_speed)
{
	Uint8* p = (Uint8*)surface->pixels;

	for (int i = MAX_STARS-1; i >= 0; --i)
	{
		StarfieldStar* star = &starfield_stars[i];

		star->position += (star->speed + move_speed) * surface->pitch;

		if (star->position < 177 * surface->pitch)
		{
			if (p[star->position] == 0)
			{
				p[star->position] = star->color;
			}

			// If star is bright enough, draw surrounding pixels
			if (star->color - 4 >= STARFIELD_HUE)
			{
				if (p[star->position + 1] == 0)
					p[star->position + 1] = star->color - 4;

				if (star->position > 0 && p[star->position - 1] == 0)
					p[star->position - 1] = star->color - 4;

				if (p[star->position + surface->pitch] == 0)
					p[star->position + surface->pitch] = star->color - 4;

				if (star->position >= surface->pitch && p[star->position - surface->pitch] == 0)
					p[star->position - surface->pitch] = star->color - 4;
			}
		}
	}
}
