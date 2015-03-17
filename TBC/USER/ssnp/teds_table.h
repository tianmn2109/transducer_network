

#ifndef TEDS_TABLE_H
#define TEDS_TABLE_H

#include <includes.h>

#include "ssnp_for_app.h"
 #include "i2c_fram.h"

#define PHY_TEDS_SIZE 19
#define META_TEDS_SIZE 35
#define TS_TEDS_SIZE 93

#pragma pack(1)

typedef struct Meta_TEDS {
    unsigned int length;         // TEDS 长度（数据块和校验和）
    unsigned char identifier;    // TEDS 标识符，标志其为Meta-TEDS
    unsigned char family;        // 1451协议族，值固定为3，表示1451.3
    unsigned char version;       // TEDS版本号 0,2-255 保留  1 第一个官方版本

    //时间相关的参数
    float teds_hold_off_time;        // 包括两个最大值，以秒为单位，第一个是，接收到READ TEDS BLOCK COMMAND 命令到将TEDS内容的第一个字节放入物理介质之间的最大延迟
                                     // 第二个是，接收到WRITE TEDS BLOCK COMMAND 命令到将WRITE TEDS BLOCK的REPLY COMMAND放入物理介质之间的最大延迟
                                     // COMMAND 可以是 Query，Read，Write，Update
    float operation_hold_off_time;   // 以秒为单位，任何不是对TEDS操作的命令到对该命令的REPLY的第一个字节被放到物理介质上

    float sustained_load_current;    // 从变送器总线得到的最大持续负载电流，以安培为单位，从任何辅助电源获得的电流不包括在内

    unsigned char multirange_capability; // 多量程的能力， 此项标志TBIM是否有能力工作在不同的量程范围内
                                        // 0 不存在这种能力  1 TBIM中的一个或者多个变送器通道具备操作在多个量程范围内的能力 2-255 保留
                                        // TEDS命令需要定义选择操作的范围并且定义变送器通道具有这个能力

    //控制组相关
    //控制组是一些变送器通道的集合，其中一个是主变送器通道，其他的是从变送器通道，从变送器通道提供关于主变送器通道的额外信息，或者用来控制主变送器通道的某些方面
    //例如：一个控制组有三个额外的与模拟事件传感器相关变送器通道，一个用来测量事件传感器的模拟输入量，一个是用来设置时间传感器阈值的执行器，一个是用来设置事件传感器滞后（hysteresis）的执行器
    unsigned char number_of_controlgroup;  //控制组的数量
    unsigned char controlgroup_type;       // 控制组的类型，此字段包含组成特定控制组变送器通道之间的关系
                                           // 0保留 1-8 使用   9-17 保留未来扩展  128-255 对制造商开放
    unsigned char num_of_ts_in_contrlgroup;   // 控制组中变送器通道的数量
    unsigned char controlgroup_list;          //??????????????????

    //向量组相关
    //向量组描述一个含有多通道的TBIM中各通道之间的关系，这些通道可以用来显示，或者这些通道算数之间的关系
    //比如三个变送器通道可以描述三维坐标轴之间的关系，将它们的结果作为向量进行显示或者进行数学计算。
    unsigned char number_of_vectorgroups;       // 向量组数量
    unsigned char vectorgroup_type;          // 向量组类型，此字段包含组成特定向量组变送器通道之间的关系
    unsigned char num_of_ts_in_vectorgroup;  // 向量组中变送器通道的数量
    unsigned char vectorgroup_list;          //??????????????????

    //变送器通道代理相关
    //变送器通道代理是一个虚拟的结构，由多个变送器通道的输出或者多个变送器通道的输入组成，变送器通道代理有可以用来读写的变送器通道号但是没有变送器通道的其他特点
    //变送器通道只支持相兼容的变送器，既不能是传感器和执行器的组合
    //两种组合变送器通道代理数据集的方法：block method可以不同长度，interleave method 必须相同长度
    unsigned char num_of_ts_proxy;           // 变送器通道代理的数量
    unsigned char ts_num_of_ts_proxy;        // 厂商定义的用来作为变送器代理的变送器通道的数量
    unsigned char ts_proxy_data_set_organization;  // 定义代理组合数据集的方法 1 块状 2 交错方法1   3 交错方法2   3-255 保留
    unsigned char ts_num_of_proxy;                 // 当前TBIM拥有的变送器通道代理的数量
    unsigned char ts__proxy_teds;                //每种代理类型变送器通道的数量
    unsigned short checksum;
}Meta_TEDS;


#pragma pack()

#pragma pack(1)
typedef struct Phy_TEDS {
    unsigned int length;           // TEDS length
    unsigned char identifier;      // TEDS 类型
    unsigned char asynchronous_only_flag;     // 非0: 只能工作在异步模式  ； 支持同步操作
    unsigned char highest_supported_payload_encoding;    // 如果接收到的帧含有保留值中的payload encoding set，则将此帧丢弃并将bad frame received 帧状态位置一
    float start_delay;          // 分配时间槽开始到传输优先延迟(the begining of the priority delay for transmission)开始之间的最大时间间隔
                                // 以s为单位，同步模式必须小于0.000025s，对于不支持同步模式的TBIM，此field不使用，设置为NaN
    float reflect_delay;        // reflect message 最后一位到达 到 reply to reply message 最后一位传输 之间的时间间隔

    float reflect_delay_uncertainty;    // reflect delay 不确定性  以s为单位
}Phy_TEDS;

#pragma pack()

#pragma pack(1)
// TransducerChannel
// 必须的，使能变送器通道寻址
typedef struct Transducerchannel_TEDS {
    unsigned int length;    // TEDS长度
    unsigned char identifier;   //  TEDS 的类型

    // 变送器通道相关信息
    unsigned char calibration_key;    // 标志通道的校验能力 0 无 1,3-6   7-255保留
                                      // 0: CAL_NONE 1: CAL_SUPPLIED (NCAP,...) 3: CAL_CUSTOM (NCAP,...)  4: CAL_TBIM_SUPPLIED (TBIM) 5: CAL_SELF (TBIM) 6: TBIM_CAL_CUSTOM (TBIM,Manufacturer-TEDS)
    unsigned char ts_type_key;        // 变送器通道类型 0 传感器  1 执行器   2 事件执行器  3-255 保留

    unsigned char physical_units;//[10];   // 应用于变送器通道数据的物理单位，如果calibration_key是CAL_SUPPLIED或者CAL_SCUSTOM，应用在传感器校验之后或者执行器校验之前
    float op_lower_range_limit;         // 传感器： 校验之后的变送器通道提供的最低有效值  执行器：变送器通道校验之前接收的最低有效值  如果低于最低有效值，则不符合制造商定义的TBIM规范，
                                        // 在不支持校验的情况下，数据可能不是单精度值，在比较之前要将数据转换成单精度值
    float op_upper_range_limit;         // 传感器： 校验之后的变送器通道提供的最高有效值  执行器：变送器通道校验之前接收的最高有效值  如果高于最高有效值，则不符合制造商定义的TBIM规范，
                                        // 在不支持校验的情况下，数据可能不是单精度值，在比较之前要将数据转换成单精度值
    float worst_uncertainty;            // 变送器通道在所有可能的环境变化的组合的情况下最坏的输出值，如（电压变化）

    unsigned char self_test;                   // 自检测能力，0 无 1 提供 2-255 保留
    unsigned char data_model;                  // Read 或者Write 变送器通道时的数据模型
                                               // 0: N-octet interger         0<=N<=8
                                               // 1: single precision real    N==4
                                               // 2: double precision real    N==8
                                               // 3: N-octet fraction         0<=N<=8
                                               // 4: Bit sequence             1
                                               // 5: long integer             9<=N<=255
                                               // 6: long fraction            9<=N<=255
                                               // 7: Time-of-Day              N=8
                                               // 8-255: reserved for future expand
    unsigned char data_model_len;              // 数据模型的长度,datamodel 的长度即N

    unsigned short model_significant_bit;      // 模型最高位位数
                                               // 假设数据来自12位的ADC，则datamodel = 0；data_model_len = 2；model_significant_bit = 12；



    unsigned short max_data_repetition;        // 最大数据重复次数，单个触发产生或者请求某个值重复的次数，每个值都代表一次测量或者执行
                                               // 当值为0的时候 series_origin, series_increment series_uint的值将被忽略
                                               // 此结构的目的是通过单次触发产生时间，频谱，或者波形
    float series_origin;                       // 当max_data_repetition的值不为0的时候，此值为产生数据集的初始值
    float series_increment;                    // 每次增长的幅度
    unsigned char series_unit; //[10];             // 物理单位

    unsigned short max_pre_trigger_samp;      // 在预触发模式下，在触发前，采样和存储变送器通道数值的次数

    float ts_update_time;                  // 接收到trigger事件到锁存变送器通道第一个采样数据之间的时间
    float ts_write_setup_time;             // 写数据帧结束到触发应用程序
    float ts_read_setup_time;              // TBIM接收到一个触发事件到数据可读
    float ts_sampling_period;              // 变送器通道采样的最小周期（不考虑读写），对于传感器和执行器，受限于A/D和D/A的转换次数，TBIM的处理速度，
    float ts_wram_up_time;                 // 上电后，变送器通道稳定需要的时间（达到预定义的容错性能）

    float ts_read_delay ;                  // 接收到Read TransducerChannel data 到开始传输数据帧 所需的时间
    float sef_test_time;                   // 执行自检测所需要的最大时间
    unsigned char source_time;             //

    float income_propagation_delay;        // 协议栈物理层接收到触发消息获取采样数据之间的时间
    float outgoing_propagation_delay;      // 最后一次采样被锁存到触发确认消息被传输之间的时间
    float triggertosample_delay;           // 触发到采样之间延迟的不确定

    unsigned char sample_attribute ;        // 变送器通道支持的采样模式
    unsigned char buffered_attribute;      // 变送器通道的带缓冲操作模式
    unsigned char end_data_set_op_attr;    // 执行器支持的end of data set operation attribute
    unsigned char streaming_attribute;     // 变送器通道是否可以支持流工作模式，0不支持，1支持，默认非流模式，2,支持，默认流模式
    unsigned char edge_to_report_attr;     // 事件传感器支持的edge to report 工作模式
    unsigned char actuator_halt_attr;      // 执行器支持的Actuator-halt工作模式
    unsigned char sensitivity_direction;   // 传感器产生正值当传感器感应的物理现象符合指定方向，（加速器，重力感应器，位移传感器，右手坐标系）

    float direction_angles;                // 与生产商规定的参考面和参考方向之间的角度
    unsigned char event_sensor_op;         // TBIM 能检测出事件传感器变化的事件
    unsigned short checksum;
}Transducerchannel_TEDS;
#pragma pack()

#define TEDS_TABLE_ADDR 0x00
#define TEDS_ENTRY_SIZE 4
#pragma pack(1)
typedef struct teds_entry 				 //存储TEDS的表项结构体定义
{
    u8 type;
	u8 valid;
	u8 addr;
	u8 len;
} teds_entry;
#pragma pack()
extern struct Meta_TEDS metaTeds;
extern struct Phy_TEDS phyTeds;
extern struct Transducerchannel_TEDS tsTeds[8];
#define TEDS_TABLE_SIZE 10				  // 可以存储TEDS的数量 共10个 1个phy teds	一个meta teds  和8个 transducer channel teds
extern teds_entry tedsTable[];   //存储	teds 的表项数组

void writeTedsEntry(struct teds_entry * entry, u8 addr);

void writeTedsTable(void); 

void initTedsTable(void);

void readTedsTable(void);

void readTeds(void);

void writeTeds(u8 * array);

void startWork(void);


#endif // TEDS_TABLE_H
