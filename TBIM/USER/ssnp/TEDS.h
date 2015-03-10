#ifndef TEDS_H
#define TEDS_H

#include "opt.h"

#ifdef PACK_STRUCT_USE_INCLUDES
#include "bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct PHY_TEDS
{
	u32t len;
	u8t id;
	u8t asy_flag;
	u8t payload_encoding;
	f32 start_delay;
	f32 reflect_delay;
	f32 reflect_delay_uncertainty;
	u16t checksum;
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#include "epstruct.h"
#endif



/********************************************************************************
 *
 *   Meta-TEDS
 *   
 *
*********************************************************************************/
#ifdef PACK_STRUCT_USE_INCLUDES
#include "bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct Meta_TEDS
{
	u32t len;
	u8t teds_id;
	u8t family_number;
	u8t version_number;

    //时间相关信息
	f32 teds_hold_off_time;
	f32 operational_hold_off_time;
	f32 load_current_drawn_frome_transducer_bus;
	u8t multirange_capability;

	//控制块相关信息
	u8t controlgroups_number;
	u8t controlgroups_type;
	u8t num_transducerchannels_in_controlgroup;
	u8t controlgroup_member_list;

	//向量组信息
	u8t vectorgroup_number;
	u8t vectorgroup_type;
	u8t num_transducerchannels_in_vectorgroup;
	u8t vectorgroup_member_list;

	//变送器通道代理定义
	u8t number_transducerchannel_proxies;
	u8t transducerchannel_number_transducerchannel_proxy;
	u8t transducerchannel_porxy_data_set_organization;
	u8t number_transducerchannels_represented_by_this_proxy;
	u8t transducerchannel_proxy_member_list;

	//校验
	u16t checksum;
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#include "epstruct.h"
#endif

/********************************************************************************
 *
 *   变送器通道TEDS
 *   
 *
*********************************************************************************/
#ifdef PACK_STRUCT_USE_INCLUDES
#include "bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct TransducerChannel_TEDS
{
	u32t len;
	u8t teds_id;

	//变送器通道相关信息
	u8t calibration_key;
	u8t transducerchannel_type_key;
	u8t physical_units[10];
	f32 design_operational_lower_range_limit;
	f32 design_operational_upper_range_limit;
	f32 worst_case_uncertainty;
	u8t self_test_key;

	//数据转换相关信息
	u8t data_mode;
	u8t data_model_length;
	u16t model_significant_bits;
	u16t maximun_data_repetitions;
	f32 series_origin;
	f32 series_increment;
	u8t series_units[10];
	u16t maximun_pretrigger_samples;

	//时间相关信息
	f32 transducerchannel_update_time;
	f32 transducerchannel_write_setup_time;
	f32 transducerchannel_read_setup_time;
	f32 transducerchannel_sampling_period;
	f32 transducerchannel_warm_up_time;
	f32 transducerchannel_read_delay_time;
	f32 transducerchannel_self_test_time_requirement;

	//采样时间相关信息
	u8t source_for_time_sample;
	f32 incoming_propagation_delay_through_data_transport_logic;
	f32 outgoing_propagation_delay_through_data_transport_logic;
	f32 trigger_to_sample_delay_uncertainty;

	//属性
	u8t sampling_attribute;
	u8t buffered_attribute;
	u8t end_of_data_set_operation_attribute;
	u8t streaming_attribute;
	u8t edge_to_report_attribute;
	u8t actuator_halt_attribute;

	//敏感度
	f32 sensitivity_direction;
	f32 direction_angles[2];

	//可选
	u8t event_sensor_options;

	//校验
	u16t checksum;
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#include "epstruct.h"
#endif


u8t phy_teds_asy_flag(void);
u8t phy_teds_payload_encoding(void);
f32 phy_teds_start_delay(void);
f32 phy_teds_reflect_delay(void);
f32 phy_teds_reflect_delay_uncertainty(void);
#endif
