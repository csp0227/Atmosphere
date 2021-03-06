/*
 * Copyright (c) 2018 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <stdint.h>

#include "memory_map.h"
#include "mc.h"
#include "exocfg.h"

typedef struct {
    uint64_t address;
    uint64_t size;
} saved_carveout_info_t;

static saved_carveout_info_t g_saved_carveouts[2] = {
    {0x80060000ull, KERNEL_CARVEOUT_SIZE_MAX},
    {0x00000000ull, 0x00000000ull}
};    

volatile security_carveout_t *get_carveout_by_id(unsigned int carveout) {
    if (CARVEOUT_ID_MIN <= carveout && carveout <= CARVEOUT_ID_MAX) {
         return (volatile security_carveout_t *)(MC_BASE + 0xC08ull + 0x50 * (carveout - CARVEOUT_ID_MIN));
    }
    generic_panic();
    return NULL;
}

void configure_gpu_ucode_carveout(void) {
    /* Starting in 6.0.0, Carveout 2 is configured later on. */
    /* This is a helper function to make this easier... */
    volatile security_carveout_t *carveout = get_carveout_by_id(2);
    carveout->paddr_low = 0x80020000;
    carveout->paddr_high = 0;
    carveout->size_big_pages = 2; /* 0x40000 */
    carveout->flags_0 = 0;
    carveout->flags_1 = 0;
    carveout->flags_2 = 0x3000000;
    carveout->flags_3 = 0;
    carveout->flags_4 = 0x300;
    carveout->flags_5 = 0;
    carveout->flags_6 = 0;
    carveout->flags_7 = 0;
    carveout->flags_8 = 0;
    carveout->flags_9 = 0;
    carveout->allowed_clients = 0x440167E;
}

void configure_default_carveouts(void) {
    /* Configure Carveout 1 (UNUSED) */
    volatile security_carveout_t *carveout = get_carveout_by_id(1);
    carveout->paddr_low = 0;
    carveout->paddr_high = 0;
    carveout->size_big_pages = 0;
    carveout->flags_0 = 0;
    carveout->flags_1 = 0;
    carveout->flags_2 = 0;
    carveout->flags_3 = 0;
    carveout->flags_4 = 0;
    carveout->flags_5 = 0;
    carveout->flags_6 = 0;
    carveout->flags_7 = 0;
    carveout->flags_8 = 0;
    carveout->flags_9 = 0;
    carveout->allowed_clients = 0x04000006;

    /* Configure Carveout 2 (GPU UCODE) */
    if (exosphere_get_target_firmware() < EXOSPHERE_TARGET_FIRMWARE_600) {
        configure_gpu_ucode_carveout();
    }

    /* Configure Carveout 3 (UNUSED GPU) */
    carveout = get_carveout_by_id(3);
    carveout->paddr_low = 0;
    carveout->paddr_high = 0;
    carveout->size_big_pages = 0;
    carveout->flags_0 = 0;
    carveout->flags_1 = 0;
    carveout->flags_2 = 0x3000000;
    carveout->flags_3 = 0;
    carveout->flags_4 = 0x300;
    carveout->flags_5 = 0;
    carveout->flags_6 = 0;
    carveout->flags_7 = 0;
    carveout->flags_8 = 0;
    carveout->flags_9 = 0;
    carveout->allowed_clients = 0x4401E7E;

    /* Configure default Kernel carveouts based on 2.0.0+. */
    if (exosphere_get_target_firmware() >= EXOSPHERE_TARGET_FIRMWARE_200) {
        /* Configure Carveout 4 (KERNEL_BUILTINS) */
        configure_kernel_carveout(4, g_saved_carveouts[0].address, g_saved_carveouts[0].size);

        /* Configure Carveout 5 (KERNEL_UNUSED) */
        configure_kernel_carveout(5, g_saved_carveouts[1].address, g_saved_carveouts[1].size);
    } else {
        for (unsigned int i = 4; i <= 5; i++) {
            carveout = get_carveout_by_id(i);
            carveout->paddr_low = 0;
            carveout->paddr_high = 0;
            carveout->size_big_pages = 0;
            carveout->flags_0 = 0;
            carveout->flags_1 = 0;
            carveout->flags_2 = 0;
            carveout->flags_3 = 0;
            carveout->flags_4 = 0;
            carveout->flags_5 = 0;
            carveout->flags_6 = 0;
            carveout->flags_7 = 0;
            carveout->flags_8 = 0;
            carveout->flags_9 = 0;
            carveout->allowed_clients = 0x4000006;
        }
    }
}

void configure_kernel_carveout(unsigned int carveout_id, uint64_t address, uint64_t size) {
    if (carveout_id != 4 && carveout_id != 5) {
        generic_panic();
    }
    
    g_saved_carveouts[carveout_id-4].address = address;
    g_saved_carveouts[carveout_id-4].size = size;

    volatile security_carveout_t *carveout = get_carveout_by_id(carveout_id);
    carveout->paddr_low = (uint32_t)(address & 0xFFFFFFFF);
    carveout->paddr_high = (uint32_t)(address >> 32);
    carveout->size_big_pages = (uint32_t)(size >> 17);
    carveout->flags_0 = 0x70E3407F;
    carveout->flags_1 = 0x1A620880;
    carveout->flags_2 = 0x303C00;
    carveout->flags_3 = 0xCF0830BB;
    carveout->flags_4 = 0x3;
    carveout->flags_5 = exosphere_get_target_firmware() >= EXOSPHERE_TARGET_FIRMWARE_400 && carveout_id == 4 ? 0x8000 : 0;
    carveout->flags_6 = exosphere_get_target_firmware() >= EXOSPHERE_TARGET_FIRMWARE_400 && carveout_id == 4 ? 0x40000 : 0;
    carveout->flags_7 = 0;
    carveout->flags_8 = 0;
    carveout->flags_9 = 0;
    carveout->allowed_clients = 0x8B;
}
