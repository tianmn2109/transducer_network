

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
    unsigned int length;         // TEDS ���ȣ����ݿ��У��ͣ�
    unsigned char identifier;    // TEDS ��ʶ������־��ΪMeta-TEDS
    unsigned char family;        // 1451Э���壬ֵ�̶�Ϊ3����ʾ1451.3
    unsigned char version;       // TEDS�汾�� 0,2-255 ����  1 ��һ���ٷ��汾

    //ʱ����صĲ���
    float teds_hold_off_time;        // �����������ֵ������Ϊ��λ����һ���ǣ����յ�READ TEDS BLOCK COMMAND �����TEDS���ݵĵ�һ���ֽڷ����������֮�������ӳ�
                                     // �ڶ����ǣ����յ�WRITE TEDS BLOCK COMMAND �����WRITE TEDS BLOCK��REPLY COMMAND�����������֮�������ӳ�
                                     // COMMAND ������ Query��Read��Write��Update
    float operation_hold_off_time;   // ����Ϊ��λ���κβ��Ƕ�TEDS����������Ը������REPLY�ĵ�һ���ֽڱ��ŵ����������

    float sustained_load_current;    // �ӱ��������ߵõ������������ص������԰���Ϊ��λ�����κθ�����Դ��õĵ�������������

    unsigned char multirange_capability; // �����̵������� �����־TBIM�Ƿ������������ڲ�ͬ�����̷�Χ��
                                        // 0 ��������������  1 TBIM�е�һ�����߶��������ͨ���߱������ڶ�����̷�Χ�ڵ����� 2-255 ����
                                        // TEDS������Ҫ����ѡ������ķ�Χ���Ҷ��������ͨ�������������

    //���������
    //��������һЩ������ͨ���ļ��ϣ�����һ������������ͨ�����������Ǵӱ�����ͨ�����ӱ�����ͨ���ṩ������������ͨ���Ķ�����Ϣ����������������������ͨ����ĳЩ����
    //���磺һ���������������������ģ���¼���������ر�����ͨ����һ�����������¼���������ģ����������һ������������ʱ�䴫������ֵ��ִ������һ�������������¼��������ͺ�hysteresis����ִ����
    unsigned char number_of_controlgroup;  //�����������
    unsigned char controlgroup_type;       // ����������ͣ����ֶΰ�������ض������������ͨ��֮��Ĺ�ϵ
                                           // 0���� 1-8 ʹ��   9-17 ����δ����չ  128-255 �������̿���
    unsigned char num_of_ts_in_contrlgroup;   // �������б�����ͨ��������
    unsigned char controlgroup_list;          //??????????????????

    //���������
    //����������һ�����ж�ͨ����TBIM�и�ͨ��֮��Ĺ�ϵ����Щͨ������������ʾ��������Щͨ������֮��Ĺ�ϵ
    //��������������ͨ������������ά������֮��Ĺ�ϵ�������ǵĽ����Ϊ����������ʾ���߽�����ѧ���㡣
    unsigned char number_of_vectorgroups;       // ����������
    unsigned char vectorgroup_type;          // ���������ͣ����ֶΰ�������ض������������ͨ��֮��Ĺ�ϵ
    unsigned char num_of_ts_in_vectorgroup;  // �������б�����ͨ��������
    unsigned char vectorgroup_list;          //??????????????????

    //������ͨ���������
    //������ͨ��������һ������Ľṹ���ɶ��������ͨ����������߶��������ͨ����������ɣ�������ͨ�������п���������д�ı�����ͨ���ŵ���û�б�����ͨ���������ص�
    //������ͨ��ֻ֧������ݵı��������Ȳ����Ǵ�������ִ���������
    //������ϱ�����ͨ���������ݼ��ķ�����block method���Բ�ͬ���ȣ�interleave method ������ͬ����
    unsigned char num_of_ts_proxy;           // ������ͨ�����������
    unsigned char ts_num_of_ts_proxy;        // ���̶����������Ϊ����������ı�����ͨ��������
    unsigned char ts_proxy_data_set_organization;  // �������������ݼ��ķ��� 1 ��״ 2 ������1   3 ������2   3-255 ����
    unsigned char ts_num_of_proxy;                 // ��ǰTBIMӵ�еı�����ͨ�����������
    unsigned char ts__proxy_teds;                //ÿ�ִ������ͱ�����ͨ��������
    unsigned short checksum;
}Meta_TEDS;


#pragma pack()

#pragma pack(1)
typedef struct Phy_TEDS {
    unsigned int length;           // TEDS length
    unsigned char identifier;      // TEDS ����
    unsigned char asynchronous_only_flag;     // ��0: ֻ�ܹ������첽ģʽ  �� ֧��ͬ������
    unsigned char highest_supported_payload_encoding;    // ������յ���֡���б���ֵ�е�payload encoding set���򽫴�֡��������bad frame received ֡״̬λ��һ
    float start_delay;          // ����ʱ��ۿ�ʼ�����������ӳ�(the begining of the priority delay for transmission)��ʼ֮������ʱ����
                                // ��sΪ��λ��ͬ��ģʽ����С��0.000025s�����ڲ�֧��ͬ��ģʽ��TBIM����field��ʹ�ã�����ΪNaN
    float reflect_delay;        // reflect message ���һλ���� �� reply to reply message ���һλ���� ֮���ʱ����

    float reflect_delay_uncertainty;    // reflect delay ��ȷ����  ��sΪ��λ
}Phy_TEDS;

#pragma pack()

#pragma pack(1)
// TransducerChannel
// ����ģ�ʹ�ܱ�����ͨ��Ѱַ
typedef struct Transducerchannel_TEDS {
    unsigned int length;    // TEDS����
    unsigned char identifier;   //  TEDS ������

    // ������ͨ�������Ϣ
    unsigned char calibration_key;    // ��־ͨ����У������ 0 �� 1,3-6   7-255����
                                      // 0: CAL_NONE 1: CAL_SUPPLIED (NCAP,...) 3: CAL_CUSTOM (NCAP,...)  4: CAL_TBIM_SUPPLIED (TBIM) 5: CAL_SELF (TBIM) 6: TBIM_CAL_CUSTOM (TBIM,Manufacturer-TEDS)
    unsigned char ts_type_key;        // ������ͨ������ 0 ������  1 ִ����   2 �¼�ִ����  3-255 ����

    unsigned char physical_units;//[10];   // Ӧ���ڱ�����ͨ�����ݵ�����λ�����calibration_key��CAL_SUPPLIED����CAL_SCUSTOM��Ӧ���ڴ�����У��֮�����ִ����У��֮ǰ
    float op_lower_range_limit;         // �������� У��֮��ı�����ͨ���ṩ�������Чֵ  ִ������������ͨ��У��֮ǰ���յ������Чֵ  ������������Чֵ���򲻷��������̶����TBIM�淶��
                                        // �ڲ�֧��У�������£����ݿ��ܲ��ǵ�����ֵ���ڱȽ�֮ǰҪ������ת���ɵ�����ֵ
    float op_upper_range_limit;         // �������� У��֮��ı�����ͨ���ṩ�������Чֵ  ִ������������ͨ��У��֮ǰ���յ������Чֵ  ������������Чֵ���򲻷��������̶����TBIM�淶��
                                        // �ڲ�֧��У�������£����ݿ��ܲ��ǵ�����ֵ���ڱȽ�֮ǰҪ������ת���ɵ�����ֵ
    float worst_uncertainty;            // ������ͨ�������п��ܵĻ����仯����ϵ������������ֵ���磨��ѹ�仯��

    unsigned char self_test;                   // �Լ��������0 �� 1 �ṩ 2-255 ����
    unsigned char data_model;                  // Read ����Write ������ͨ��ʱ������ģ��
                                               // 0: N-octet interger         0<=N<=8
                                               // 1: single precision real    N==4
                                               // 2: double precision real    N==8
                                               // 3: N-octet fraction         0<=N<=8
                                               // 4: Bit sequence             1
                                               // 5: long integer             9<=N<=255
                                               // 6: long fraction            9<=N<=255
                                               // 7: Time-of-Day              N=8
                                               // 8-255: reserved for future expand
    unsigned char data_model_len;              // ����ģ�͵ĳ���,datamodel �ĳ��ȼ�N

    unsigned short model_significant_bit;      // ģ�����λλ��
                                               // ������������12λ��ADC����datamodel = 0��data_model_len = 2��model_significant_bit = 12��



    unsigned short max_data_repetition;        // ��������ظ���������������������������ĳ��ֵ�ظ��Ĵ�����ÿ��ֵ������һ�β�������ִ��
                                               // ��ֵΪ0��ʱ�� series_origin, series_increment series_uint��ֵ��������
                                               // �˽ṹ��Ŀ����ͨ�����δ�������ʱ�䣬Ƶ�ף����߲���
    float series_origin;                       // ��max_data_repetition��ֵ��Ϊ0��ʱ�򣬴�ֵΪ�������ݼ��ĳ�ʼֵ
    float series_increment;                    // ÿ�������ķ���
    unsigned char series_unit; //[10];             // ����λ

    unsigned short max_pre_trigger_samp;      // ��Ԥ����ģʽ�£��ڴ���ǰ�������ʹ洢������ͨ����ֵ�Ĵ���

    float ts_update_time;                  // ���յ�trigger�¼������������ͨ����һ����������֮���ʱ��
    float ts_write_setup_time;             // д����֡����������Ӧ�ó���
    float ts_read_setup_time;              // TBIM���յ�һ�������¼������ݿɶ�
    float ts_sampling_period;              // ������ͨ����������С���ڣ������Ƕ�д�������ڴ�������ִ������������A/D��D/A��ת��������TBIM�Ĵ����ٶȣ�
    float ts_wram_up_time;                 // �ϵ�󣬱�����ͨ���ȶ���Ҫ��ʱ�䣨�ﵽԤ������ݴ����ܣ�

    float ts_read_delay ;                  // ���յ�Read TransducerChannel data ����ʼ��������֡ �����ʱ��
    float sef_test_time;                   // ִ���Լ������Ҫ�����ʱ��
    unsigned char source_time;             //

    float income_propagation_delay;        // Э��ջ�������յ�������Ϣ��ȡ��������֮���ʱ��
    float outgoing_propagation_delay;      // ���һ�β��������浽����ȷ����Ϣ������֮���ʱ��
    float triggertosample_delay;           // ����������֮���ӳٵĲ�ȷ��

    unsigned char sample_attribute ;        // ������ͨ��֧�ֵĲ���ģʽ
    unsigned char buffered_attribute;      // ������ͨ���Ĵ��������ģʽ
    unsigned char end_data_set_op_attr;    // ִ����֧�ֵ�end of data set operation attribute
    unsigned char streaming_attribute;     // ������ͨ���Ƿ����֧��������ģʽ��0��֧�֣�1֧�֣�Ĭ�Ϸ���ģʽ��2,֧�֣�Ĭ����ģʽ
    unsigned char edge_to_report_attr;     // �¼�������֧�ֵ�edge to report ����ģʽ
    unsigned char actuator_halt_attr;      // ִ����֧�ֵ�Actuator-halt����ģʽ
    unsigned char sensitivity_direction;   // ������������ֵ����������Ӧ�������������ָ�����򣬣���������������Ӧ����λ�ƴ���������������ϵ��

    float direction_angles;                // �������̹涨�Ĳο���Ͳο�����֮��ĽǶ�
    unsigned char event_sensor_op;         // TBIM �ܼ����¼��������仯���¼�
    unsigned short checksum;
}Transducerchannel_TEDS;
#pragma pack()

#define TEDS_TABLE_ADDR 0x00
#define TEDS_ENTRY_SIZE 4
#pragma pack(1)
typedef struct teds_entry 				 //�洢TEDS�ı���ṹ�嶨��
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
#define TEDS_TABLE_SIZE 10				  // ���Դ洢TEDS������ ��10�� 1��phy teds	һ��meta teds  ��8�� transducer channel teds
extern teds_entry tedsTable[];   //�洢	teds �ı�������

void writeTedsEntry(struct teds_entry * entry, u8 addr);

void writeTedsTable(void); 

void initTedsTable(void);

void readTedsTable(void);

void readTeds(void);

void writeTeds(u8 * array);

void startWork(void);


#endif // TEDS_TABLE_H
