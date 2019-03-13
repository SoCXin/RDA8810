/*
 *  audiocontrol class driver
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
*/

#ifndef __LINUX_AUDIOCONTROL_H__
#define __LINUX_AUDIOCONTROL_H__

struct audiocontrol_dev {
	const char	*name;
	struct device	*dev;
	int		index;
	int		volume;
	int		mute;

	ssize_t	(*get_volume)(struct audiocontrol_dev *sdev);
	ssize_t	(*set_volume)(struct audiocontrol_dev *sdev, int volume);
	ssize_t	(*get_mute)(struct audiocontrol_dev *sdev);
	ssize_t	(*set_mute)(struct audiocontrol_dev *sdev, int mute);
};

extern int audiocontrol_dev_register(struct audiocontrol_dev *sdev);
extern void audiocontrol_dev_unregister(struct audiocontrol_dev *sdev);

#endif /* __LINUX_AUDIOCONTROL_H__ */
