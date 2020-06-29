/*
	This file is part of fsdump, a tool for dumping drives into image files.
	Copyright (C) 2020 Simon Gander.

	FSDump is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	FSDump is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with fsdump.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <cstdint>

typedef int64_t paddr_t;
typedef uint64_t oid_t;
typedef uint64_t xid_t;

struct prange_t {
	paddr_t pr_start_addr;
	uint64_t pr_block_count;
};

typedef unsigned char apfs_uuid_t[16];

struct obj_phys_t {
	uint64_t o_cksum;
	oid_t o_oid;
	xid_t o_xid;
	uint32_t o_type;
	uint32_t o_subtype;
};

#define OID_NX_SUPERBLOCK 1
#define OID_INVALID 0ULL
#define OID_RESERVED_COUNT 1024

#define OBJECT_TYPE_MASK                0x0000FFFF
#define OBJECT_TYPE_FLAGS_MASK          0xFFFF0000

#define OBJ_STORAGETYPE_MASK            0xC0000000
#define OBJECT_TYPE_FLAGS_DEFINED_MASK  0xF8000000

#define OBJECT_TYPE_NX_SUPERBLOCK       0x00000001
#define OBJECT_TYPE_BTREE               0x00000002
#define OBJECT_TYPE_BTREE_NODE          0x00000003
#define OBJECT_TYPE_SPACEMAN            0x00000005
#define OBJECT_TYPE_SPACEMAN_CAB        0x00000006
#define OBJECT_TYPE_SPACEMAN_CIB        0x00000007
#define OBJECT_TYPE_SPACEMAN_BITMAP     0x00000008
#define OBJECT_TYPE_SPACEMAN_FREE_QUEUE 0x00000009
#define OBJECT_TYPE_EXTENT_LIST_TREE    0x0000000A
#define OBJECT_TYPE_OMAP                0x0000000B
#define OBJECT_TYPE_CHECKPOINT_MAP      0x0000000C
#define OBJECT_TYPE_FS                  0x0000000D
#define OBJECT_TYPE_FSTREE              0x0000000E
#define OBJECT_TYPE_BLOCKREFTREE        0x0000000F
#define OBJECT_TYPE_SNAPMETATREE        0x00000010
#define OBJECT_TYPE_NX_REAPER           0x00000011
#define OBJECT_TYPE_NX_REAP_LIST        0x00000012
#define OBJECT_TYPE_OMAP_SNAPSHOT       0x00000013
#define OBJECT_TYPE_EFI_JUMPSTART       0x00000014
#define OBJECT_TYPE_FUSION_MIDDLE_TREE  0x00000015
#define OBJECT_TYPE_NX_FUSION_WBC       0x00000016
#define OBJECT_TYPE_NX_FUSION_WBC_LIST  0x00000017
#define OBJECT_TYPE_ER_STATE            0x00000018
#define OBJECT_TYPE_GBITMAP             0x00000019
#define OBJECT_TYPE_GBITMAP_TREE        0x0000001A
#define OBJECT_TYPE_GBITMAP_BLOCK       0x0000001B
#define OBJECT_TYPE_ER_RECOVERY_BLOCK   0x0000001C
#define OBJECT_TYPE_SNAP_META_EXT       0x0000001D
#define OBJECT_TYPE_INTEGRITY_META      0x0000001E
#define OBJECT_TYPE_FEXT_TREE           0x0000001F
#define OBJECT_TYPE_RESERVED_20         0x00000020

#define OBJECT_TYPE_INVALID             0x00000000
#define OBJECT_TYPE_TEST                0x000000FF

#define OBJECT_TYPE_CONTAINER_KEYBAG    'keys'
#define OBJECT_TYPE_VOLUME_KEYBAG       'recs'
#define OBJECT_TYPE_MEDIA_KEYBAG        'mkey'

#define OBJ_VIRTUAL       0x00000000
#define OBJ_EPHEMERAL     0x80000000
#define OBJ_PHYSICAL      0x40000000

#define OBJ_NOHEADER      0x20000000
#define OBJ_ENCRYPTED     0x10000000
#define OBJ_NONPERSISTENT 0x08000000


#define NX_MAGIC                        'BSXN'
#define NX_MAX_FILE_SYSTEMS             100

#define NX_EPH_INFO_COUNT               4
#define NX_EPH_MIN_BLOCK_COUNT          4
#define NX_MAX_FILE_SYSTEM_EPH_STRUCTS  4
#define NX_TX_MIN_CHECKPOINT_COUNT      4
#define NX_EPH_INFO_VERSION_1           1

#define NX_RESERVED_1                   0x00000001LL
#define NX_RESERVED_2                   0x00000002LL
#define NX_CRYPTO_SW                    0x00000004LL

#define NX_FEATURE_DEFRAG               0x0000000000000001ULL
#define NX_FEATURE_LCFD                 0x0000000000000002ULL
#define NX_SUPPORTED_FEATURES_MASK      (NX_FEATURE_DEFRAG | NX_FEATURE_LCFD)

#define NX_SUPPORTED_ROCOMPAT_MASK      (0x0ULL)

#define NX_INCOMPAT_VERSION1            0x0000000000000001ULL
#define NX_INCOMPAT_VERSION2            0x0000000000000002ULL
#define NX_INCOMPAT_FUSION              0x0000000000000100ULL
#define NX_SUPPORTED_INCOMPAT_MASK      (NX_INCOMPAT_VERSION2 | NX_INCOMPAT_FUSION)

#define NX_MINIMUM_BLOCK_SIZE           4096
#define NX_DEFAULT_BLOCK_SIZE           4096
#define NX_MAXIMUM_BLOCK_SIZE           65536

#define NX_MINIMUM_CONTAINER_SIZE       1048576

enum nx_counter_id_t
{
	NX_CNTR_OBJ_CKSUM_SET = 0,
	NX_CNTR_OBJ_CKSUM_FAIL = 1,

	NX_NUM_COUNTERS = 32
};

struct nx_superblock_t
{
	obj_phys_t  nx_o;
	uint32_t    nx_magic;
	uint32_t    nx_block_size;
	uint64_t    nx_block_count;

	uint64_t    nx_features;
	uint64_t    nx_readonly_compatible_features;
	uint64_t    nx_incompatible_features;

	apfs_uuid_t nx_uuid;

	oid_t       nx_next_oid;
	xid_t       nx_next_xid;

	uint32_t    nx_xp_desc_blocks;
	uint32_t    nx_xp_data_blocks;
	paddr_t     nx_xp_desc_base;
	paddr_t     nx_xp_data_base;
	uint32_t    nx_xp_desc_next;
	uint32_t    nx_xp_data_next;
	uint32_t    nx_xp_desc_index;
	uint32_t    nx_xp_desc_len;
	uint32_t    nx_xp_data_index;
	uint32_t    nx_xp_data_len;

	oid_t       nx_spaceman_oid;
	oid_t       nx_omap_oid;
	oid_t       nx_reaper_oid;

	uint32_t    nx_test_type;

	uint32_t    nx_max_file_systems;
	oid_t       nx_fs_oid[NX_MAX_FILE_SYSTEMS];
	uint64_t    nx_counters[NX_NUM_COUNTERS];
	prange_t    nx_blocked_out_prange;
	oid_t       nx_evict_mapping_tree_oid;
	uint64_t    nx_flags;
	paddr_t     nx_efi_jumpstart;
	apfs_uuid_t nx_fusion_uuid;
	prange_t    nx_keylocker;
	uint64_t    nx_ephemeral_info[NX_EPH_INFO_COUNT];

	oid_t       nx_test_oid;

	oid_t       nx_fusion_mt_oid;
	oid_t       nx_fusion_wbc_oid;
	prange_t    nx_fusion_wbc;

	uint64_t    nx_newest_mounted_version;

	prange_t    nx_mkb_locker;
};

struct checkpoint_mapping_t
{
	uint32_t    cpm_type;
	uint32_t    cpm_subtype;
	uint32_t    cpm_size;
	uint32_t    cpm_pad;
	oid_t       cpm_fs_oid;
	oid_t       cpm_oid;
	paddr_t     cpm_paddr;
};

struct checkpoint_map_phys_t
{
	obj_phys_t             cpm_o;
	uint32_t               cpm_flags;
	uint32_t               cpm_count;
	checkpoint_mapping_t   cpm_map[];
};

#define CHECKPOINT_MAP_LAST 0x00000001

struct chunk_info_t
{
	uint64_t    ci_xid;
	uint64_t    ci_addr;
	uint32_t    ci_block_count;
	uint32_t    ci_free_count;
	paddr_t     ci_bitmap_addr;
};

struct chunk_info_block_t
{
	obj_phys_t      cib_o;
	uint32_t        cib_index;
	uint32_t        cib_chunk_info_count;
	chunk_info_t    cib_chunk_info[];
};

struct cib_addr_block_t
{
	obj_phys_t  cab_o;
	uint32_t    cab_index;
	uint32_t    cab_cib_count;
	paddr_t     cab_cib_addr[];
};

struct spaceman_free_queue_t
{
	uint64_t    sfq_count;
	oid_t       sfq_tree_oid;
	xid_t       sfq_oldest_xid;
	uint16_t    sfq_tree_node_limit;
	uint16_t    sfq_pad16;
	uint32_t    sfq_pad32;
	uint64_t    sfq_reserved;
};

struct spaceman_device_t
{
	uint64_t    sm_block_count;
	uint64_t    sm_chunk_count;
	uint32_t    sm_cib_count;
	uint32_t    sm_cab_count;
	uint64_t    sm_free_count;
	uint32_t    sm_addr_offset;
	uint32_t    sm_reserved;
	uint64_t    sm_reserved2;
};

struct spaceman_allocation_zone_boundaries_t
{
	uint64_t saz_zone_start;
	uint64_t saz_zone_end;
};

#define SM_ALLOCZONE_INVALID_END_BOUNDARY      0
#define SM_ALLOCZONE_NUM_PREVIOUS_BOUNDARIES   7

struct spaceman_allocation_zone_info_phys_t
{
	spaceman_allocation_zone_boundaries_t   saz_current_boundaries;
	spaceman_allocation_zone_boundaries_t   saz_previous_boundaries[SM_ALLOCZONE_NUM_PREVIOUS_BOUNDARIES];
	uint16_t saz_zone_id;
	uint16_t saz_previous_boundary_index;
	uint32_t saz_reserved;
};

enum sfq {
	SFQ_IP = 0,
	SFQ_MAIN = 1,
	SFQ_TIER2 = 2,
	SFQ_COUNT = 3
};

enum smdev {
	SD_MAIN = 0,
	SD_TIER2 = 1,
	SD_COUNT = 2
};

#define SM_DATAZONE_ALLOCZONE_COUNT 8

struct spaceman_datazone_info_phys_t
{
	spaceman_allocation_zone_info_phys_t sdz_allocation_zones[SD_COUNT][SM_DATAZONE_ALLOCZONE_COUNT];
};

struct spaceman_phys_t
{
	obj_phys_t                      sm_o;
	uint32_t                        sm_block_size;
	uint32_t                        sm_blocks_per_chunk;
	uint32_t                        sm_chunks_per_cib;
	uint32_t                        sm_cibs_per_cab;
	spaceman_device_t               sm_dev[SD_COUNT];
	uint32_t                        sm_flags;
	uint32_t                        sm_ip_bm_tx_multiplier;
	uint64_t                        sm_ip_block_count;
	uint32_t                        sm_ip_bm_size_in_blocks;
	uint32_t                        sm_ip_bm_block_count;
	paddr_t                         sm_ip_bm_base;
	paddr_t                         sm_ip_base;
	uint64_t                        sm_fs_reserve_block_count;
	uint64_t                        sm_fs_reserve_alloc_count;
	spaceman_free_queue_t           sm_fq[SFQ_COUNT];
	uint16_t                        sm_ip_bm_free_head;
	uint16_t                        sm_ip_bm_free_tail;
	uint32_t                        sm_ip_bm_xid_offset;
	uint32_t                        sm_ip_bitmap_offset;
	uint32_t                        sm_ip_bm_free_next_offset;
	uint32_t                        sm_version;
	uint32_t                        sm_struct_size;
	spaceman_datazone_info_phys_t   sm_datazone;
};
