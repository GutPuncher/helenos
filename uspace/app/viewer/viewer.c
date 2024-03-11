/*
 * Copyright (c) 2024 Jiri Svoboda
 * Copyright (c) 2013 Martin Decky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup viewer
 * @{
 */
/** @file
 */

#include <errno.h>
#include <gfximage/tga.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <ui/image.h>
#include <ui/ui.h>
#include <ui/wdecor.h>
#include <ui/window.h>
#include <vfs/vfs.h>

#define NAME  "viewer"

typedef struct {
	ui_t *ui;

	size_t imgs_count;
	size_t imgs_current;
	char **imgs;

	ui_window_t *window;
	gfx_bitmap_t *bitmap;
	ui_image_t *image;
	gfx_context_t *window_gc;

	gfx_rect_t img_rect;
} viewer_t;

static bool viewer_img_load(viewer_t *, const char *, gfx_bitmap_t **,
    gfx_rect_t *);
static bool viewer_img_setup(viewer_t *, gfx_bitmap_t *, gfx_rect_t *);

static void wnd_close(ui_window_t *, void *);
static void wnd_kbd_event(ui_window_t *, void *, kbd_event_t *);

static ui_window_cb_t window_cb = {
	.close = wnd_close,
	.kbd = wnd_kbd_event
};

/** Window close request
 *
 * @param window Window
 * @param arg Argument (calc_t *)
 */
static void wnd_close(ui_window_t *window, void *arg)
{
	viewer_t *viewer = (viewer_t *) arg;

	ui_quit(viewer->ui);
}

static void wnd_kbd_event(ui_window_t *window, void *arg,
    kbd_event_t *event)
{
	viewer_t *viewer = (viewer_t *)arg;
	bool update = false;

	if ((event->type == KEY_PRESS) && (event->c == 'q'))
		ui_quit(viewer->ui);

	if ((event->type == KEY_PRESS) && (event->key == KC_PAGE_DOWN)) {
		if (viewer->imgs_current == viewer->imgs_count - 1)
			viewer->imgs_current = 0;
		else
			viewer->imgs_current++;

		update = true;
	}

	if ((event->type == KEY_PRESS) && (event->key == KC_PAGE_UP)) {
		if (viewer->imgs_current == 0)
			viewer->imgs_current = viewer->imgs_count - 1;
		else
			viewer->imgs_current--;

		update = true;
	}

	if (update) {
		gfx_bitmap_t *lbitmap;
		gfx_rect_t lrect;

		if (!viewer_img_load(viewer, viewer->imgs[viewer->imgs_current],
		    &lbitmap, &lrect)) {
			printf("Cannot load image \"%s\".\n",
			    viewer->imgs[viewer->imgs_current]);
			exit(4);
		}
		if (!viewer_img_setup(viewer, lbitmap, &lrect)) {
			printf("Cannot setup image \"%s\".\n",
			    viewer->imgs[viewer->imgs_current]);
			exit(6);
		}
	}
}

static bool viewer_img_load(viewer_t *viewer, const char *fname,
    gfx_bitmap_t **rbitmap, gfx_rect_t *rect)
{
	int fd;
	errno_t rc = vfs_lookup_open(fname, WALK_REGULAR, MODE_READ, &fd);
	if (rc != EOK)
		return false;

	vfs_stat_t stat;
	rc = vfs_stat(fd, &stat);
	if (rc != EOK) {
		vfs_put(fd);
		return false;
	}

	void *tga = malloc(stat.size);
	if (tga == NULL) {
		vfs_put(fd);
		return false;
	}

	size_t nread;
	rc = vfs_read(fd, (aoff64_t []) { 0 }, tga, stat.size, &nread);
	if (rc != EOK || nread != stat.size) {
		free(tga);
		vfs_put(fd);
		return false;
	}

	vfs_put(fd);

	rc = decode_tga(viewer->window_gc, tga, stat.size, rbitmap, rect);
	if (rc != EOK) {
		free(tga);
		return false;
	}

	free(tga);

	viewer->img_rect = *rect;
	return true;
}

static bool viewer_img_setup(viewer_t *viewer, gfx_bitmap_t *bmp,
    gfx_rect_t *rect)
{
	gfx_rect_t arect;
	gfx_rect_t irect;
	ui_resource_t *ui_res;
	errno_t rc;

	ui_res = ui_window_get_res(viewer->window);

	ui_window_get_app_rect(viewer->window, &arect);

	/* Center image on application area */
	gfx_rect_ctr_on_rect(rect, &arect, &irect);

	if (viewer->image != NULL) {
		ui_image_set_bmp(viewer->image, bmp, rect);
		(void) ui_image_paint(viewer->image);
		ui_image_set_rect(viewer->image, &irect);
	} else {
		rc = ui_image_create(ui_res, bmp, rect, &viewer->image);
		if (rc != EOK) {
			gfx_bitmap_destroy(bmp);
			return false;
		}

		ui_image_set_rect(viewer->image, &irect);
		ui_window_add(viewer->window, ui_image_ctl(viewer->image));
	}

	if (viewer->bitmap != NULL)
		gfx_bitmap_destroy(viewer->bitmap);

	viewer->bitmap = bmp;
	return true;
}

static void print_syntax(void)
{
	printf("Syntax: %s [<options] <image-file>...\n", NAME);
	printf("\t-d <display-spec> Use the specified display\n");
	printf("\t-f                Full-screen mode\n");
}

int main(int argc, char *argv[])
{
	const char *display_spec = UI_ANY_DEFAULT;
	gfx_bitmap_t *lbitmap;
	gfx_rect_t lrect;
	bool fullscreen = false;
	gfx_rect_t rect;
	gfx_rect_t wrect;
	gfx_coord2_t off;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	viewer_t *viewer;
	errno_t rc;
	int i;
	unsigned u;

	viewer = calloc(1, sizeof(viewer_t));
	if (viewer == NULL) {
		printf("Out of memory.\n");
		goto error;
	}

	i = 1;
	while (i < argc && argv[i][0] == '-') {
		if (str_cmp(argv[i], "-d") == 0) {
			++i;
			if (i >= argc) {
				printf("Argument missing.\n");
				print_syntax();
				goto error;
			}

			display_spec = argv[i++];
		} else if (str_cmp(argv[i], "-f") == 0) {
			++i;
			fullscreen = true;
		} else {
			printf("Invalid option '%s'.\n", argv[i]);
			print_syntax();
			goto error;
		}
	}

	if (i >= argc) {
		printf("No image files specified.\n");
		print_syntax();
		goto error;
	}

	viewer->imgs_count = argc - i;
	viewer->imgs = calloc(viewer->imgs_count, sizeof(char *));
	if (viewer->imgs == NULL) {
		printf("Out of memory.\n");
		goto error;
	}

	for (int j = 0; j < argc - i; j++) {
		viewer->imgs[j] = str_dup(argv[i + j]);
		if (viewer->imgs[j] == NULL) {
			printf("Out of memory.\n");
			goto error;
		}
	}

	rc = ui_create(display_spec, &ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n", display_spec);
		goto error;
	}

	if (ui_is_fullscreen(ui))
		fullscreen = true;

	viewer->ui = ui;

	/*
	 * We don't know the image size yet, so create tiny window and resize
	 * later.
	 */
	ui_wnd_params_init(&params);
	params.caption = "Viewer";
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p1.x = 1;
	params.rect.p1.y = 1;

	if (fullscreen) {
		params.style &= ~ui_wds_decorated;
		params.placement = ui_wnd_place_full_screen;
	}

	rc = ui_window_create(ui, &params, &viewer->window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		goto error;
	}

	viewer->window_gc = ui_window_get_gc(viewer->window);

	ui_window_set_cb(viewer->window, &window_cb, (void *)viewer);

	if (!viewer_img_load(viewer, viewer->imgs[viewer->imgs_current],
	    &lbitmap, &lrect)) {
		printf("Cannot load image \"%s\".\n",
		    viewer->imgs[viewer->imgs_current]);
		goto error;
	}

	/*
	 * Compute window rectangle such that application area corresponds
	 * to rect
	 */
	ui_wdecor_rect_from_app(ui, params.style, &lrect, &wrect);
	off = wrect.p0;
	gfx_rect_rtranslate(&off, &wrect, &rect);

	if (!fullscreen) {
		rc = ui_window_resize(viewer->window, &rect);
		if (rc != EOK) {
			printf("Error resizing window.\n");
			goto error;
		}
	}

	if (!viewer_img_setup(viewer, lbitmap, &lrect)) {
		printf("Cannot setup image \"%s\".\n",
		    viewer->imgs[viewer->imgs_current]);
		goto error;
	}

	rc = ui_window_paint(viewer->window);
	if (rc != EOK) {
		printf("Error painting window.\n");
		goto error;
	}

	ui_run(ui);

	ui_window_destroy(viewer->window);
	ui_destroy(ui);
	free(viewer);

	return 0;
error:
	if (viewer != NULL && viewer->imgs != NULL) {
		for (u = 0; u < viewer->imgs_count; u++) {
			if (viewer->imgs[i] != NULL)
				free(viewer->imgs[i]);
		}
		free(viewer->imgs);
	}
	if (viewer != NULL && viewer->window != NULL)
		ui_window_destroy(viewer->window);
	if (ui != NULL)
		ui_destroy(ui);
	if (viewer != NULL)
		free(viewer);
	return 1;
}

/** @}
 */
