#
# Copyright (C) 2007 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

TARGET_PLATFORM := $(TARGET_BOARD_PLATFORM)
TARGET_BOARD := slt

# enable internal storage by virtual FAT partition on nand
TARGET_FAT_ON_NAND := false

# enable internal storage by FAT partition with NFTL
TARGET_FAT_NFTL := false

TARGET_SYSTEMIMAGE_USE_EXT4 := true
TARGET_USERIMAGES_USE_EXT4 := true

BOARD_SYSTEMIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE := ext4

PRODUCT_NAME := $(TARGET_BOARD)
PRODUCT_DEVICE := $(TARGET_BOARD)
PRODUCT_MODEL := RDA SmartPhone
PRODUCT_BRAND := RDA
PRODUCT_MANUFACTURER := Rdamicro
