/*
 * libwebsockets-DTS2B-client - libwebsockets DTS2B implementation
 *
 * Copyright (C) 2015 mleaf_hexi <350983773@qq.com>
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include </usr/local/include/json/json.h>//json��ͷ�ļ�
#include <uuid/uuid.h>//����UUID���ͷ�ļ�
#include<sqlite3.h>//sqlite3���ļ�
char *errmsg=0;



#ifdef CMAKE_BUILD
#include "lws_config.h"
#endif

#include <libwebsockets.h>

static unsigned int opts;
static int was_closed;
static int deny_deflate;
static int deny_mux;
static struct libwebsocket *wsi_mirror;
static int mirror_lifetime = 0;
static volatile int force_exit = 0;
static int longlived = 0;
/*�豸״̬��غ궨��*/
#define ONLINE 1          //���ߣ���ʾ �豸������ ���ҹ�������
#define TROUBLE 2        // ��ʾ �豸�����ӣ����ǹ���������
#define SHUTDOWNING 3   //��ʾ �豸�����ӣ����ǹ���������
#define REBOOTING 4    //��ʾ �豸��������
#define BOOTING 5     //��ʾ �豸��������
#define SHUTDOWN 6   //��ʾ �豸�ѹر�
#define UPGRADING 7 //��ʾ �豸��������
#define DISABLED 8 //��ʾ �豸�Ѿ�������
#define UNKNOWN 9 //��ʾ �豸״̬����֪

/*�豸����-DeviceType*/
#define DTS_ROUTER	1 //���ֽ�ѧһ��� - ·����
#define DTS_EMBEDDED_COMPUTER 2	//���ֽ�ѧһ��� - ��Ƕ PC
#define DTS_PROJECTOR 3	//���� DTS ��ͶӰ��
#define DTS_DISPLAYER 4	//���� DTS ����ʾ�豸
#define DTS_SWITCH 5	//����
#define DTS_RFID_READER 6 //RFID������
#define DTS_SENSOR 7	//������
#define DTS_ALL_PERIPHERY_DEVICE 8	//DTS ��������
/*�豸������-ConfigOption*/

	/*������ƽ̨ ���� URL ������ {deviceId}��
	DTSͨ��URL �� schema �ж��Ƿ�ʹ�ð�ȫ���ӣ�
	�� wss ʹ�ð�ȫ���ӡ������ö������URL,
	��ʹ�÷ָ���Ӣ�ﶺ�ŷָ�, 
	DTS �豸 Ӧ���� ���� ��һ�� URL��
	���ʧ�ܲ����еڶ���URL ��Ӧ�������ӵڶ��� URL�� 
	�Դ�����.*/
#define CLOUD_PLATFORM_WEBSOCKET_URL 192.168.1.123
/*DTS �ϱ��豸״̬ Ƶ�ʣ���λ �롣 ��һ���ϱ�ʱ�� Ӧ�����*/
#define REPORT_DTS_DEVICE_STATUS_PERIOD 72
/*DTS �ϴ�����־ Ƶ��, ��λ �롣��һ���ϴ���־��ʱ�� �ں����ʱ����������*/
#define UPLOAD_DTS_LOG_DATA_PERIOD 12 
/*DTS ����־����, ���ܵ�ֵ�� ALL,DEBUG, INFO, WARN, ERROR,FATAL,OFF*/
#define DTS_LOG_LEVEL 1
/*DTS �ϱ��豸��ǰ������Ϣ Ƶ�ʣ���λ �롣 ��һ���ϱ�ʱ�� Ӧ�����*/
#define REPORT_DTS_DEVICE_RUNTIME_INFO_PERIOD 12
/*��ѧ���ڿ�ʼʱ��,��ֵ��ʾ����һ�쿪ʼ(00:00)�ĺ�����*/
#define CHECK_IN_START_TIME 200
/*��ѧ���ڽ���ʱ��,��ֵ��ʾ����һ�쿪ʼ(00:00)�ĺ�����*/
#define CHECK_IN_END_TIME 200
/*��ѧ���ڿ�ʼʱ��,��ֵ��ʾ����һ�쿪ʼ(00:00)�ĺ�����*/
#define CHECK_OUT_START_TIME 200
/*��ѧ���ڽ���ʱ��,��ֵ��ʾ����һ�쿪ʼ(00:00)�ĺ�����*/
#define CHECK_OUT_END_TIME 200

/*sqlite3��غ���*/
/*
*�������ݵ�sqlite3 ���ݿ�
*author  mleaf_hexi
*mail:350983773@qq.com
*/
void insert_data(sqlite3 *db)
{
    int number,age,score;
    char name[20];
 
    printf("input the number: ");
    scanf("%d",&number);
    getchar();
    printf("input the name: ");
    scanf("%s",name);
    getchar();
    printf("input the age: ");
    scanf("%d",&age);
    getchar();
    printf("input the score ");
    scanf("%d",&score);
    getchar();
    char *sql=sqlite3_mprintf("insert into student values('%d','%s','%d','%d')",number,name,age,score);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
    {
        perror("sqlite3_exec");
        exit(-1);
    }
    else
        printf("insert success!!\n");
    return;
}
 
 /*
 *ɾ��sqlite3 ���ݿ�number λ�õ�����
 *author  mleaf_hexi
 *mail:350983773@qq.com
 */
void delete_data(sqlite3 *db)
{
    int num;
    printf("please input the number you want to delete\n");
    scanf("%d",&num);
    getchar();
	//ɾ����student���ݿ��number��ͷ��
    char *sql=sqlite3_mprintf("delete from student where number ='%d'",num);
 
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
    {
        perror("sqlite3_exec_delete");
        exit(-1);
    }
    else
        printf("delete success!!\n");
    return;
 
}
 
 /*
 *����sqlite3 ���ݿ�����
 *author  mleaf_hexi
 *mail:350983773@qq.com
 */
void updata_data(sqlite3 * db)
{
    int num,age,score;
    char name[20];
 
    printf("please input the number you want to updata\n");
    scanf("%d",&num);
    getchar();
    printf("input the name: ");
    scanf("%s",name);
    getchar();
    printf("input the age: ");
    scanf("%d",&age);
    getchar();
    printf("input the score ");
    scanf("%d",&score);
    getchar();
 
//  char sql[1024];
//  sprintf(sql,"update student set name='%s',age='%d',score='%d' where number='%d'",name,age,score,num);
    char *sql=sqlite3_mprintf("update student set name='%s',age='%d',score='%d' where number='%d'",name,age,score,num);
 
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
    {
        perror("sqlite3_exec_update");
        exit(-1);
    }
    else
        printf("update success!!\n");
    return;
 
}
/*
*��ʾsqlite3 ���ݿ�����
*author  mleaf_hexi
*mail:350983773@qq.com
*/
 
void show_data(sqlite3 *db)
{
    char ** resultp;
    int nrow,ncolumn,i,j,index;
    char *sql="select * from student";
	/*
		int sqlite3_get_table(
	  	sqlite3 *db,          //An open database 
	  	const char *zSql,    // SQL to be evaluated 
	  	char ***pazResult,   // Results of the query
	  	
	  	int *pnRow,           //Number of result rows written here
	  	int *pnColumn,        //Number of result columns written here
	  	char **pzErrmsg      //Error msg written here 
	);
	*/

    if(sqlite3_get_table(db,sql,&resultp,&nrow,&ncolumn,&errmsg) != SQLITE_OK)
    {
        perror("sqlite3_get_table");
        exit(-1);
    }
 
    index = ncolumn;
    for(i = 0 ;i< ncolumn; i++)
	{
	    printf("%s\t",resultp[i]);
		printf("\n");
	}
 
    for(i = 0;i< nrow;i++)
    {
        for(j = 0;j< ncolumn;j++)
        printf("%s\t",resultp[index++]);
        printf("\n");
    }
    return ;
}
/*
*�˳�sqlite3 ���ݿ�
*author  mleaf_hexi
*mail:350983773@qq.com
*/

void quit(sqlite3 *db)
{
    printf("BYBYE!!\n");
    sqlite3_close(db);
    exit(0);
}
/*
*sqlite3 ���ݿ���Գ���
*author  mleaf_hexi
*mail:350983773@qq.com
*/

int sqlite3_test(void)
{
    int nu;
    sqlite3 *db;
	int  ret = 0;
    if(sqlite3_open("brxydata.db",&db) != SQLITE_OK)
    {
        perror("sqlite3_open");
        exit(-1);
    }
	//�����ȴ������ݿ�� ��Ȼ������ӳ�Ա
	//�������ݿ��
    const char *SQL1="create table student(number,name varchar(20),age,score );";
/*int sqlite3_exec(
*	  sqlite3* ppDb,                                          //An open database 
*	  const char *sql,                                        //SQL to be evaluated 
* 	  int (*callback)(void*,int,char**,char**), //Callback function 
*	  void *,                                                    //1st argument to callback 
*	  char **errmsg                                        // Error msg written here
*	);
*/
	//ִ�н���
    ret = sqlite3_exec(db,SQL1,0,0,&errmsg);
    if(ret != SQLITE_OK)
    {
        fprintf(stderr,"SQL Error:%s\n",errmsg);
        sqlite3_free(errmsg);
    }
 
   /* while(1)
    {
        printf("**********************\n");
        printf("*   1.insert data    *\n");
        printf("*   2.delete data    *\n");
        printf("*   3.updata data    *\n");
        printf("*   4.show   data    *\n");
        printf("*   5. quit          *\n");
        printf("**********************\n");
        printf("please input the number you want to operate:\n");
        scanf("%d",&nu);
        getchar();
        switch(nu)
        {
        case 1:
            insert_data(db);//�������
            break;
        case 2:
            delete_data(db);//ɾ������
            break;
        case 3:
            updata_data(db);//��������
            break;
        case 4:
            show_data(db);//��ʾ����
            break;
        case 5 :
            quit(db);//�˳�
            break;
        }
    }*/
    return 0;
}


/*
*��ȡ�����UUID
*author  mleaf_hexi
*mail:350983773@qq.com
*/
static unsigned char* uuidget(char str[36])
{
    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse(uuid, str);
    printf("%s\n", str);
    return str;
}


/*
*2.1.1	�ϱ�DTS״̬
*��������: unsigned char*
*����ֵ: json��ʽ����
*author  mleaf_hexi
*mail:350983773@qq.com
*/
static unsigned char* reportDeviceStatus(void)
{
	char struuidget[36];
	int L = 0;
	int n;
	unsigned char*reportDeviceStatus_string;
	unsigned char reportDeviceStatus_buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
								  LWS_SEND_BUFFER_POST_PADDING];
	json_object *mainjson=json_object_new_object();
	json_object *header=json_object_new_object();
	json_object *action=json_object_new_object();

	
	//header
	json_object *action1=json_object_new_array();
	json_object *requestId=json_object_new_array();
	json_object_array_add(action1,json_object_new_string("REPORT_DTS_DEVICE_STATUS"));
	uuidget(struuidget);//��ȡUUID��struuidget
	printf("uuidget = %s\n", struuidget);
	json_object_array_add(requestId,json_object_new_string(struuidget));
	json_object_object_add(action,"action",action1);
	json_object_object_add(action,"requestId",requestId);
	json_object_object_add(mainjson,"header",action);
	//data
// "deviceType1": "<DeviceType1>", "status"
	json_object *status1=json_object_new_object();
	json_object_object_add(status1,
	   "deviceIdentify",json_object_new_string("DeviceIdentifyCode11"));
	json_object_object_add(status1,
	   "deviceStatus",json_object_new_string("<DeviceStatus11>"));
	
	json_object *status11=json_object_new_object();
	json_object_object_add(status11,
	   "deviceIdentify",json_object_new_string("DeviceIdentifyCode12"));
	json_object_object_add(status11,
	   "deviceStatus",json_object_new_string("<DeviceStatus12>"));

	// "deviceType2": "<DeviceType2>", "status"
	
	json_object *status2=json_object_new_object();
	json_object_object_add(status2,
	   "deviceIdentify",json_object_new_string("DeviceIdentifyCode21"));
	json_object_object_add(status2,
	   "deviceStatus",json_object_new_string("<DeviceStatus21>"));

	json_object *status22=json_object_new_object();
	json_object_object_add(status22,
	   "deviceIdentify",json_object_new_string("DeviceIdentifyCode22"));
	json_object_object_add(status22,
	   "deviceStatus",json_object_new_string("<DeviceStatus22>"));


	
	json_object *status3=json_object_new_array();
	json_object_array_add(status3,status1);
	json_object_array_add(status3,status11);
	
	
	
	const char *status3_str=json_object_to_json_string(status3);
	
	printf("status3_str=%s\n",status3_str);
	/*
	[
	    {
	        "deviceIdentify": "DeviceIdentifyCode11",
	        "deviceStatus": "<DeviceStatus11>"
	    },
	    {
	        "deviceIdentify": "DeviceIdentifyCode12",
	        "deviceStatus": "<DeviceStatus12>"
	    }
	]

	*/

	json_object *status5=json_object_new_array();
		json_object_array_add(status5,status2);
	json_object_array_add(status5,status22);
	const char *status5_str=json_object_to_json_string(status5);
	
	printf("status5_str=%s\n",status5_str);
	
	/*
	[
	    {
	        "deviceIdentify": "DeviceIdentifyCode21",
	        "deviceStatus": "<DeviceStatus21>"
	    },
	    {
	        "deviceIdentify": "DeviceIdentifyCode22",
	        "deviceStatus": "<DeviceStatus22>"
	    }
	]
	*/

	json_object *status4=json_object_new_object();
	json_object_object_add(status4, "deviceType1",json_object_new_string("<DeviceType1>"));
	json_object_object_add(status4,"status",status3);
	
	
	const char *status4_str=json_object_to_json_string(status4);
	
	printf("status4_str=%s\n",status4_str);

	/*
		{
	    "deviceType1": "<DeviceType1>",
	    "status": [
	        {
	            "deviceIdentify": "DeviceIdentifyCode11",
	            "deviceStatus": "<DeviceStatus11>"
	        },
	        {
	            "deviceIdentify": "DeviceIdentifyCode12",
	            "deviceStatus": "<DeviceStatus12>"
	        }
		    ]
		}

	*/
	json_object *status6=json_object_new_object();
	json_object_object_add(status6, "deviceType2",json_object_new_string("<DeviceType2>"));
	json_object_object_add(status6,"status",status5);
	
	
	const char *status6_str=json_object_to_json_string(status6);
	
	printf("status6_str=%s\n",status6_str);
	/*
	{
	    "deviceType2": "<DeviceType2>",
	    "status": [
	        {
	            "deviceIdentify": "DeviceIdentifyCode21",
	            "deviceStatus": "<DeviceStatus21>"
	        },
	        {
	            "deviceIdentify": "DeviceIdentifyCode22",
	            "deviceStatus": "<DeviceStatus22>"
	        }
	    ]
	}
	*/
	

	json_object *data=json_object_new_object();
	json_object *data1=json_object_new_array();
	json_object_array_add(data1,status4);
	json_object_array_add(data1,status6);
	json_object_object_add(mainjson,"data",data1);

	const char *str=json_object_to_json_string(mainjson);
	printf("%s\n",str);
	/*
	{
    "header": {
        "action": [
            "REPORT_DTS_DEVICE_STATUS"
        ],
        "requestId": [
            "825b2533-e507-4708-8c0b-eb77f62fe596"
        ]
    },
    "data": [
        {
            "deviceType1": "<DeviceType1>",
            "status": [
                {
                    "deviceIdentify": "DeviceIdentifyCode11",
                    "deviceStatus": "<DeviceStatus11>"
                },
                {
                    "deviceIdentify": "DeviceIdentifyCode12",
                    "deviceStatus": "<DeviceStatus12>"
                }
            ]
        },
        {
            "deviceType2": "<DeviceType2>",
            "status": [
                {
                    "deviceIdentify": "DeviceIdentifyCode21",
                    "deviceStatus": "<DeviceStatus21>"
                },
                {
                    "deviceIdentify": "DeviceIdentifyCode22",
                    "deviceStatus": "<DeviceStatus22>"
                }
            ]
        }
    ]
}
	*/

	printf("%s\n",str);
	for (n = 0; n < 1; n++)//ת����libwebsocket ���Է��͵�����
		   L += sprintf((char *)&reportDeviceStatus_buf[LWS_SEND_BUFFER_PRE_PADDING + L],"%s",json_object_to_json_string(mainjson));
	reportDeviceStatus_string=&reportDeviceStatus_buf[LWS_SEND_BUFFER_PRE_PADDING];
	
	json_object_put(mainjson);
	json_object_put(header);
	json_object_put(action);
	json_object_put(action1);
	json_object_put(requestId);
	json_object_put(status1);
	json_object_put(status11);
	json_object_put(status2);
	json_object_put(status22);
	json_object_put(status3);
	json_object_put(status4);
	json_object_put(status5);
	json_object_put(status6);
	json_object_put(data);
	json_object_put(data1);	
	return reportDeviceStatus_string;
	
}

/*
* 2.1.2	�ϴ�DTS��־
*��������: unsigned char*
*����ֵ: json��ʽ����
*author  mleaf_hexi
*mail:350983773@qq.com
*/
static unsigned char* reportDeviceLog(void) 
{
	char struuidget[36];
	int L = 0;
	int n;
	unsigned char*reportDeviceLog_string;
	unsigned char DeviceLog_buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
								  LWS_SEND_BUFFER_POST_PADDING];
	json_object *mainjson=json_object_new_object();
	json_object *header=json_object_new_object();
	json_object *action=json_object_new_object();
	json_object *data=json_object_new_object();
	
	
	json_object *action1=json_object_new_array();
	json_object *requestId=json_object_new_array();
	json_object_array_add(action1,json_object_new_string("REPORT_DTS_DEVICE_LOG"));
	uuidget(struuidget);//��ȡUUID��struuidget
	printf("uuidget = %s\n", struuidget);
	json_object_array_add(requestId,json_object_new_string(struuidget));//��ȡUUIDд��requestId
	json_object_object_add(action,"action",action1);
	json_object_object_add(action,"requestId",requestId);
	json_object_object_add(mainjson,"header",action);


	json_object_object_add(data,
	   "fromDatetime",json_object_new_int64(9223372036854775807));
	json_object_object_add(data,
	   "toDatetime",json_object_new_int(39));
	json_object_object_add(data,
	   "logData",json_object_new_string("Logdatafromdtsdevice1asdfaew"));
	json_object_object_add(mainjson,"data",data);

	
	const unsigned char *str=json_object_to_json_string(mainjson);
	printf("%s\n",str);
	for (n = 0; n < 1; n++)//ת����libwebsocket ���Է��͵�����
		   L += sprintf((char *)&DeviceLog_buf[LWS_SEND_BUFFER_PRE_PADDING + L],"%s",json_object_to_json_string(mainjson));
	reportDeviceLog_string=&DeviceLog_buf[LWS_SEND_BUFFER_PRE_PADDING];
	json_object_put(action);//�ڳ�������ͷ���Դ
	json_object_put(action1);
	json_object_put(header);
	json_object_put(requestId);
	json_object_put(data);
	json_object_put(mainjson);
	
	return reportDeviceLog_string;
	
}

/*
*2.1.3	�ϱ�DTS������Ϣ
*��������: unsigned char*
*����ֵ: json��ʽ����
*author  mleaf_hexi
*mail:350983773@qq.com
*/

static unsigned char* reportDeviceRuntimeInfo(void)
{
		char struuidget[36];
		int L = 0;
		int n;
		unsigned char*reportDeviceRuntimeInfo_string;
		unsigned char reportDeviceRuntimeInfo_buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
									  LWS_SEND_BUFFER_POST_PADDING];
		json_object *mainjson=json_object_new_object();
		json_object *header=json_object_new_object();
		json_object *action=json_object_new_object();
	
		
		//header
		json_object *action1=json_object_new_array();
		json_object *requestId=json_object_new_array();
		json_object_array_add(action1,json_object_new_string("REPORT_DTS_DEVICE_RUNTIME_INFO"));
		uuidget(struuidget);//��ȡUUID��struuidget
		printf("uuidget = %s\n", struuidget);
		json_object_array_add(requestId,json_object_new_string(struuidget));
		json_object_object_add(action,"action",action1);
		json_object_object_add(action,"requestId",requestId);
		json_object_object_add(mainjson,"header",action);
		//data
	// "deviceType1": "<DeviceType1>", "idenfities"
		json_object *status1=json_object_new_object();
		json_object_object_add(status1,
		   "deviceProperty",json_object_new_string("<DeviceProperty1>"));
		json_object_object_add(status1,
		   "propertyValue",json_object_new_string("PropertyValue1"));
		
		json_object *status11=json_object_new_object();
		json_object_object_add(status11,
		   "deviceProperty",json_object_new_string("<DeviceProperty2>"));
		json_object_object_add(status11,
		   "propertyValue",json_object_new_string("PropertyValue2"));
	
		// "deviceType2": "<DeviceType2>", "status"
		
		json_object *status2=json_object_new_object();
		json_object_object_add(status2,
		   "deviceProperty",json_object_new_string("<DeviceProperty3>"));
		json_object_object_add(status2,
		   "propertyValue",json_object_new_string("PropertyValue3"));
	
		json_object *status22=json_object_new_object();
		json_object_object_add(status22,
		   "deviceProperty",json_object_new_string("<DeviceProperty4>"));
		json_object_object_add(status22,
		   "propertyValue",json_object_new_string("PropertyValue4"));
	
	
		
		json_object *status3=json_object_new_array();
		json_object_array_add(status3,status1);
		json_object_array_add(status3,status11);
		
		
		
		const char *status3_str=json_object_to_json_string(status3);
		
		printf("status3_str=%s\n",status3_str);
		/*
			[
			{
				"deviceProperty": "<DeviceProperty1>",
				"propertyValue": "PropertyValue1"
			},
			{
				"deviceProperty": "<DeviceProperty2>",
				"propertyValue": "PropertyValue2"
			}
		]
	
		*/
	
		json_object *status5=json_object_new_array();
		json_object_array_add(status5,status2);
		json_object_array_add(status5,status22);
		const char *status5_str=json_object_to_json_string(status5);
		
		printf("status5_str=%s\n",status5_str);
		
		/*
			[
			{
				"deviceProperty": "<DeviceProperty3>",
				"propertyValue": "PropertyValue3"
			},
			{
				"deviceProperty": "<DeviceProperty4>",
				"propertyValue": "PropertyValue4"
			}
		]
		*/
	
		json_object *status4=json_object_new_object();
		json_object_object_add(status4, "deviceIdentify",json_object_new_string("DeviceIdentifyCode1"));
		json_object_object_add(status4,"informations",status3);
	
		json_object *status4A=json_object_new_array();
		json_object_array_add(status4A,status4);
		
		json_object *status4B=json_object_new_object();
		json_object_object_add(status4B, "deviceType",json_object_new_string("<DeviceType1>"));
		json_object_object_add(status4B,"idenfities",status4A);
		
		const char *status4B_str=json_object_to_json_string(status4B);
		
		printf("status4B_str=%s\n",status4B_str);
	
		/*
			{
			"deviceType": "<DeviceType1>",
			"idenfities": [
				{
					"deviceIdentify": "DeviceIdentifyCode1",
					"informations": [
						{
							"deviceProperty": "<DeviceProperty1>",
							"propertyValue": "PropertyValue1"
						},
						{
							"deviceProperty": "<DeviceProperty2>",
							"propertyValue": "PropertyValue2"
						}
						]
					}
			  ]
			}
	
		*/
		json_object *status6=json_object_new_object();
		json_object_object_add(status6, "deviceIdentify",json_object_new_string("DeviceIdentifyCode2"));
		json_object_object_add(status6,"informations",status5);
	
		json_object *status6A=json_object_new_array();
		json_object_array_add(status6A,status6);
	
		
		json_object *status6B=json_object_new_object();
		json_object_object_add(status6B, "deviceType",json_object_new_string("<DeviceType2>"));
		json_object_object_add(status6B,"idenfities",status6A);
		
		const char *status6B_str=json_object_to_json_string(status6B);
		
		printf("status6B_str=%s\n",status6B_str);
		/*
			{
			"deviceType": "<DeviceType2>",
			"idenfities": [
				{
					"deviceIdentify": "DeviceIdentifyCode2",
					"informations": [
						{
							"deviceProperty": "<DeviceProperty3>",
							"propertyValue": "PropertyValue3"
						},
						{
							"deviceProperty": "<DeviceProperty4>",
							"propertyValue": "PropertyValue4"
							}
						]
					}
				]
			}
		*/
		
	
		json_object *data=json_object_new_object();
		json_object *data1=json_object_new_array();
		json_object_array_add(data1,status4B);
		json_object_array_add(data1,status6B);
		json_object_object_add(mainjson,"data",data1);
	
		const char *str=json_object_to_json_string(mainjson);
		printf("%s\n",str);
		/*
		{
		"header": {
			"action": [
				"REPORT_DTS_DEVICE_RUNTIME_INFO"
			],
			"requestId": [
				"bf77a870-1d28-4530-814d-8ec6fe8a9b5f"
			]
		},
		"data": [
			{
				"deviceType": "<DeviceType1>",
				"idenfities": [
					{
						"deviceIdentify": "DeviceIdentifyCode1",
						"informations": [
							{
								"deviceProperty": "<DeviceProperty1>",
								"propertyValue": "PropertyValue1"
							},
							{
								"deviceProperty": "<DeviceProperty2>",
								"propertyValue": "PropertyValue2"
							}
						]
					}
				]
			},
			{
				"deviceType": "<DeviceType2>",
				"idenfities": [
					{
						"deviceIdentify": "DeviceIdentifyCode2",
						"informations": [
							{
								"deviceProperty": "<DeviceProperty3>",
								"propertyValue": "PropertyValue3"
							},
							{
								"deviceProperty": "<DeviceProperty4>",
								"propertyValue": "PropertyValue4"
							}
						]
					}
				]
			}
		]
	}
		*/


		printf("%s\n",str);
		for (n = 0; n < 1; n++)//ת����libwebsocket ���Է��͵�����
		   L += sprintf((char *)&reportDeviceRuntimeInfo_buf[LWS_SEND_BUFFER_PRE_PADDING + L],"%s",json_object_to_json_string(mainjson));
		reportDeviceRuntimeInfo_string=&reportDeviceRuntimeInfo_buf[LWS_SEND_BUFFER_PRE_PADDING];

		json_object_put(mainjson);
		json_object_put(header);
		json_object_put(action);
		json_object_put(action1);
		json_object_put(requestId);
		json_object_put(status1);
		json_object_put(status11);
		json_object_put(status2);
		json_object_put(status22);
		json_object_put(status3);
		json_object_put(status4);
		json_object_put(status4A);
		json_object_put(status4B);
		json_object_put(status5);
		json_object_put(status6);
		json_object_put(status6A);
		json_object_put(status6B);
		json_object_put(data);
		json_object_put(data1);
		return reportDeviceRuntimeInfo_string;

}


/*
*2.1.4	����DTS�豸
*��������: unsigned char*
*����ֵ: json��ʽ����
*author  mleaf_hexi
*mail:350983773@qq.com
*/
static unsigned char* requestActiveDtsDevice()
{
		char struuidget[36];
		int L = 0;
		int n;
		unsigned char*requestActiveDtsDevice_string;
		unsigned char requestActiveDtsDevice_buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
									  LWS_SEND_BUFFER_POST_PADDING];
		json_object *mainjson=json_object_new_object();
		json_object *header=json_object_new_object();
		json_object *action=json_object_new_object();
		json_object *data=json_object_new_object();
		
		
		json_object *action1=json_object_new_array();
		json_object *requestId=json_object_new_array();
		json_object_array_add(action1,json_object_new_string("REPORT_DTS_DEVICE_LOG"));
		uuidget(struuidget);//��ȡUUID��struuidget
		printf("uuidget = %s\n", struuidget);
		json_object_array_add(requestId,json_object_new_string(struuidget));
	
		json_object *activeCode=json_object_new_array();
		json_object_array_add(activeCode,json_object_new_string("DTS Active code"));
		
		json_object_object_add(action,"action",action1);
		json_object_object_add(action,"requestId",requestId);
		json_object_object_add(action,"activeCode",activeCode);
		json_object_object_add(mainjson,"header",action);
	
	
		const char *str=json_object_to_json_string(mainjson);
		printf("%s\n",str);
				for (n = 0; n < 1; n++)//ת����libwebsocket ���Է��͵�����
				   L += sprintf((char *)&requestActiveDtsDevice_buf[LWS_SEND_BUFFER_PRE_PADDING + L],"%s",json_object_to_json_string(mainjson));
		requestActiveDtsDevice_string=&requestActiveDtsDevice_buf[LWS_SEND_BUFFER_PRE_PADDING];
		json_object_put(action);
		json_object_put(action1);
		json_object_put(header);
		json_object_put(requestId);
		json_object_put(activeCode);
		json_object_put(mainjson);
		return requestActiveDtsDevice_string;
	
	
	/*�����ʽ
	{
		"header": {
			"action": [
				"REPORT_DTS_DEVICE_LOG"
			],
			"requestId": [
				"d4cbed46-3283-4ec3-b9e2-2bdde32914ee"
			],
			"activeCode": [
				"DTS Active code"
			]
		}
	}
	*/
	/*
	���ز���
	
	{
	"headers":{
		"action": ["REQUEST_ACTIVE_DTS_DEVICE"],
		"requestId": [""]	--���ֵ, ����������Ψһ. ���ڼ�������, ֧�ֵ��ַ� a-zA-Z0-9_-
	},
	 "statusCode":"<StatusCode>", ������
	 "data":{
		 "accessCode":"DTS Access Code" ����ɹ��� �� ������.
		 "failReason": "***", -- ʧ��ԭ��
	}
	}
	*/


}


/*
*2.1.5	�ϱ�RFID ˢ������
*��������: unsigned char*
*����ֵ: json��ʽ����
*author  mleaf_hexi
*mail:350983773@qq.com
*/
static unsigned char*reportDtsRfidCheckOnData()
{
		char struuidget[36];
		int L = 0;
		int n;
		unsigned char*reportDtsRfidCheckOnData_string;
		unsigned char reportDtsRfidCheckOnData_buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
									  LWS_SEND_BUFFER_POST_PADDING];
		json_object *mainjson=json_object_new_object();
		json_object *header=json_object_new_object();
		json_object *action=json_object_new_object();
		
		json_object *action1=json_object_new_array();
		json_object *requestId=json_object_new_array();
		json_object_array_add(action1,json_object_new_string("REPORT_DTS_DEVICE_LOG"));
		uuidget(struuidget);//��ȡUUID��struuidget
		printf("uuidget = %s\n", struuidget);
		json_object_array_add(requestId,json_object_new_string(struuidget));
	
		json_object_object_add(action,"action",action1);
		json_object_object_add(action,"requestId",requestId);
		json_object_object_add(mainjson,"header",action);
	
	
		json_object *data1=json_object_new_object();
		json_object_object_add(data1,
		   "cardId",json_object_new_string("RFID Card ID1"));
		json_object_object_add(data1,
		   "checkOnType",json_object_new_string("<CheckOnType1>"));
		json_object_object_add(data1,
		   "swipDatetime",json_object_new_string("Logdatafromdtsdevice1asdfaew1"));
	
		json_object *data2=json_object_new_object();
		json_object_object_add(data2,
		   "cardId",json_object_new_string("RFID Card ID2"));
		json_object_object_add(data2,
		   "checkOnType",json_object_new_string("<CheckOnType2>"));
		json_object_object_add(data2,
		   "swipDatetime",json_object_new_string("Logdatafromdtsdevice1asdfaew2"));
	
	
		json_object *data=json_object_new_array();
		json_object_array_add(data,data1);
		json_object_array_add(data,data2);
	
		json_object_object_add(mainjson,"data",data);
		const char *str=json_object_to_json_string(mainjson);
		printf("%s\n",str);
		
		for (n = 0; n < 1; n++)//ת����libwebsocket ���Է��͵�����
		   L += sprintf((char *)&reportDtsRfidCheckOnData_buf[LWS_SEND_BUFFER_PRE_PADDING + L],"%s",json_object_to_json_string(mainjson));
		reportDtsRfidCheckOnData_string=&reportDtsRfidCheckOnData_buf[LWS_SEND_BUFFER_PRE_PADDING];

		json_object_put(action);
		json_object_put(action1);
		json_object_put(header);
		json_object_put(requestId);
		json_object_put(data);
		json_object_put(data1);
		json_object_put(data2);
		json_object_put(mainjson);
	
	return reportDtsRfidCheckOnData_string;
	/*�����ʽ
	{
		"header": {
			"action": [
				"REPORT_DTS_DEVICE_LOG"
			],
			"requestId": [
				"878e0a2c-a412-479b-b3e1-ccfdcafed02b"
			]
		},
		"data": [
			{
				"cardId": "RFID Card ID1",
				"checkOnType": "<CheckOnType1>",
				"swipDatetime": "Logdatafromdtsdevice1asdfaew1"
			},
			{
				"cardId": "RFID Card ID2",
				"checkOnType": "<CheckOnType2>",
				"swipDatetime": "Logdatafromdtsdevice1asdfaew2"
			}
		]
	}
	
	*/


}



/*
*
*2.1.6	����ͬ��DTS����
*��������: unsigned char*
*����ֵ: json��ʽ����
*author  mleaf_hexi
*mail:350983773@qq.com
*
*/
static unsigned char* RequestSyncDtsAllConfigOptions(void)
{
		char struuidget[36];
		int L = 0;
		int n;
		unsigned char*RequestSyncDtsAllConfigOptions_string;
		unsigned char RequestSyncDtsAllConfigOptions_buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
									  LWS_SEND_BUFFER_POST_PADDING];
		json_object *mainjson=json_object_new_object();
		json_object *header=json_object_new_object();
		json_object *action=json_object_new_object();
		
		json_object *action1=json_object_new_array();
		json_object *requestId=json_object_new_array();
		json_object_array_add(action1,json_object_new_string("REQUEST_SYNC_DTS_ALL_CONFIG_OPTIONS"));
		uuidget(struuidget);//��ȡUUID��struuidget
		printf("uuidget = %s\n", struuidget);
		json_object_array_add(requestId,json_object_new_string(struuidget));
	
		json_object_object_add(action,"action",action1);
		json_object_object_add(action,"requestId",requestId);
		json_object_object_add(mainjson,"header",action);
		
		const char *str=json_object_to_json_string(mainjson);
		printf("%s\n",str);
		
		for (n = 0; n < 1; n++)//ת����libwebsocket ���Է��͵�����
			L += sprintf((char *)&RequestSyncDtsAllConfigOptions_buf[LWS_SEND_BUFFER_PRE_PADDING + L],"%s",json_object_to_json_string(mainjson));
		RequestSyncDtsAllConfigOptions_string=&RequestSyncDtsAllConfigOptions_buf[LWS_SEND_BUFFER_PRE_PADDING];
		json_object_put(mainjson);
		json_object_put(header);
		json_object_put(action);
		json_object_put(action1);
		json_object_put(requestId);
		return RequestSyncDtsAllConfigOptions_string;

/*�����ʽ
{
    "header": {
        "action": [
            "REQUEST_SYNC_DTS_ALL_CONFIG_OPTIONS"
        ],
        "requestId": [
            "157ed038-1c6d-44f4-a036-1d52bb9314f3"
        ]
    }
}

*/


}

/*
*2.1.7	����ͬ��RFIDȨ��
*��������: unsigned char*
*����ֵ: json��ʽ����
*author  mleaf_hexi
*mail:350983773@qq.com
*/

static unsigned char* RequestSyncDtsAllRfidPermission(void)
{
		char struuidget[36];
		int L = 0;
		int n;
		unsigned char*RequestSyncDtsAllRfidPermission_string;
		unsigned char RequestSyncDtsAllRfidPermission_buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
									  LWS_SEND_BUFFER_POST_PADDING];
		json_object *mainjson=json_object_new_object();
		json_object *header=json_object_new_object();
		json_object *action=json_object_new_object();
		
		json_object *action1=json_object_new_array();
		json_object *requestId=json_object_new_array();
		json_object_array_add(action1,json_object_new_string("REQUEST_SYNC_DTS_ALL_RFID_PERMISSION"));
		uuidget(struuidget);//��ȡUUID��struuidget
		printf("uuidget = %s\n", struuidget);
		json_object_array_add(requestId,json_object_new_string(struuidget));
	
		json_object_object_add(action,"action",action1);
		json_object_object_add(action,"requestId",requestId);
		json_object_object_add(mainjson,"header",action);
		
		const char *str=json_object_to_json_string(mainjson);
		printf("%s\n",str);
		
		for (n = 0; n < 1; n++)//ת����libwebsocket ���Է��͵�����
			L += sprintf((char *)&RequestSyncDtsAllRfidPermission_buf[LWS_SEND_BUFFER_PRE_PADDING + L],"%s",json_object_to_json_string(mainjson));
		RequestSyncDtsAllRfidPermission_string=&RequestSyncDtsAllRfidPermission_buf[LWS_SEND_BUFFER_PRE_PADDING];
		json_object_put(mainjson);
		json_object_put(header);
		json_object_put(action);
		json_object_put(action1);
		json_object_put(requestId);
		return RequestSyncDtsAllRfidPermission_string;
	
	/*�����ʽ
	{
		"header": {
			"action": [
				"REQUEST_SYNC_DTS_ALL_CONFIG_OPTIONS"
			],
			"requestId": [
				"157ed038-1c6d-44f4-a036-1d52bb9314f3"
			]
		}
	}
	
	*/

}


/*
* JSON��ʽ���ݲ���
*��������: unsigned char*
*����ֵ: json��ʽ����
*/

static unsigned char* json_text_test(void)
{	
	struct json_object *my_string;
	int L = 0;
	int n;
	unsigned char*json_string;
	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
								  LWS_SEND_BUFFER_POST_PADDING];

	my_string = json_object_new_string("mleafhx");

	/*��� my_string=   */
	printf("my_string=%s\n", json_object_get_string(my_string));
	/*ת��json��ʽ�ַ��� ���my_string.to_string()="\t"*/
	printf("my_string.to_string()=%s\n", json_object_to_json_string(my_string)); 
	for (n = 0; n < 1; n++)
		   L += sprintf((char *)&buf[LWS_SEND_BUFFER_PRE_PADDING + L],"%s",json_object_to_json_string(my_string));
	json_string=&buf[LWS_SEND_BUFFER_PRE_PADDING];
	/*�ͷ���Դ*/
	json_object_put(my_string);

	return json_string;

}
/*
*text�ı����ݲ���
*��������: unsigned char*
*����ֵ: json��ʽ����
*/

static unsigned char* text_send_test(void)
{
		int L = 0;
		int n;
		unsigned char*text_string;
		unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
									  LWS_SEND_BUFFER_POST_PADDING];

	for (n = 0; n < 1; n++)
				L+= sprintf((char *)&buf[LWS_SEND_BUFFER_PRE_PADDING + L],"%s","MLEAF hexi 350983773@qq.com");
	text_string=&buf[LWS_SEND_BUFFER_PRE_PADDING];
	return text_string;

}
/*json��ʽ������غ���*/
static void json_print_value(json_object *obj);
static void json_print_array(json_object *obj);
static void json_print_object(json_object *obj);

/*
*��ӡJSON���ݵ�ֵ
*��������: none
*����ֵ: none
*�������json_object *����
*author  mleaf_hexi
*mail:350983773@qq.com
*/ 
static void json_print_value(json_object *obj) 
{
	char *action="CONFIG_DTS_DEVICE_OPTIONS";
	char *configOption="CLOUD_PLATFORM_WEBSOCKET_URL";

	if(!obj) return;
	json_type type=json_object_get_type(obj);
	if(type == json_type_boolean) {
	    if(json_object_get_boolean(obj)) {
	        printf("true");
	    } else {
	        printf("false");
	    }
	} else if(type == json_type_double) {
	    printf("json_object_get_double=%lf\n",json_object_get_double(obj));
	} else if(type == json_type_int) {
	    printf("json_object_get_int=%d\n",json_object_get_int(obj));
	} 
	else if(type == json_type_string) 
	{
	    
		if(strcmp(json_object_get_string(obj),action)==0)
		{
			printf("action checking out\n");
			printf("action=%s\n",json_object_get_string(obj));
		}
		else if(strcmp(json_object_get_string(obj),configOption)==0)
		{
			printf("configOption checking out\n");
			printf("configOption=%s\n",json_object_get_string(obj));

		}
		else
		{
			printf("json_object_get_string=%s\n",json_object_get_string(obj));
		}
	} 
	else if(type == json_type_object) {
	    json_print_object(obj);
	} else if(type == json_type_array) {
	    json_print_array(obj);
	} else {
	    printf("ERROR");
	}
	printf(" ");
}
 /*
*��ӡJSON�������͵�ֵ
*��������: none
*����ֵ: none
*�������json_object *����
*author  mleaf_hexi
*mail:350983773@qq.com
*/ 
static void json_print_array(json_object *obj) 
{
	int i;

	if(!obj) return;
    	
    int length=json_object_array_length(obj);
    for(i=0;i<length;i++) 
	{
        json_object *val=json_object_array_get_idx(obj,i);
        json_print_value(val);
    }
}
/*
*��ӡJSON object���͵�ֵ
*��������: none
*����ֵ: none
*�������json_object *����
*author  mleaf_hexi
*mail:350983773@qq.com
*/ 
static void json_print_object(json_object *obj) 
{
	char *configData="configData";
	char *requestId="requestId";
	
	char *delim=":/,";//�ָ��ַ���
	char *p;
	char *s;
	char *name,*value,*next,*value1,*next1,*next2;
	int i;
	char buff[50],buff2[50];
	if(!obj) return;
	//����json�����key��ֵ 
	//Linux�ں�2.6.29��˵����strtok()�Ѿ�����ʹ�ã����ٶȸ����strsep()���档
	json_object_object_foreach(obj,key,val) 
	{
		//printf("%s => ",key);
		if(strcmp(key,configData)==0)//ȡ��ͨѶ��url
		{
			printf("configData checking out\n");
			printf("configData=%s\n",json_object_get_string(val));
			
			s=(char*)json_object_get_string(val);
			printf("%s\n",s);
//wss: //area1.dts.mpush.brxy-cloud.com/websocket/connHandler/v2.0,wss: //primary.dts.mpush.brxy-cloud.com/websocket/connHandler/v2.0
			
			value = strdup(s);

			for(i=0 ;i<2 ;i++)
			{ // ��һ��ִ��ʱ
				name = strsep(&value,":"); // ��":"�ָ��ַ���,��ʱstrsep��������ֵΪ "wss",��":"��֮ǰ���ַ���
				next =value; // ��ʱָ��valueָ��":"�ź�����ַ���,�� //area1.dts.mpush.brxy-cloud.com/websocket/connHandler/v2.0,wss: //primary.dts.mpush.brxy-cloud.com/websocket/connHandler/v2.0
				printf(" name= %s\n",name); //��ӡ��һ�ַָ��name��ֵ
				name = strsep(&value,"/");// ��ʱͨ��"/"�ָ��ַ���
				next =value; 
				name = strsep(&value,"/");//ȥ���ڶ���/
				next =value; 
				if(i==0)
				{
					value=strsep(&next,",");// ��","�ָ��ַ���,��ʱstrsep��������ֵΪ "area1.dts.mpush.brxy-cloud.com/websocket/connHandler/v2.0",��","��֮ǰ���ַ���
					printf("value= %s\n",value);
					next2 =value;
					value=strsep(&next2,"/");
					printf("value= %s\n",value);//area1.dts.mpush.brxy-cloud.com
					sprintf(buff2,"/%s",next2);
					printf("next2= %s\n",buff2);/* /websocket/connHandler/v2.0	 */ 
				}
				if(i==1)//�ڶ���ѭ��
				{
					value1=strsep(&next,"");// ��/0�ָ��ַ���,��ʱstrsep��������ֵΪ "primary.dts.mpush.brxy-cloud.com/websocket/connHandler/v2.0",��wss: //֮��"/0"֮ǰ���ַ���
					printf("value1= %s\n",value1);
					next1 =value1;
					value1=strsep(&next1,"/");
					printf("value2= %s\n",value1);//primary.dts.mpush.brxy-cloud.com
					sprintf(buff,"/%s",next1);
					printf("next1= %s\n",buff);/* /websocket/connHandler/v2.0  */

				}
				value=next;
			}
//			char *source = strdup(s); 
//			char *token;  
//			for(token = strsep(&source, delim); token != NULL; token = strsep(&source, delim)) 
//			{  
//				printf("%s",token);  
//				printf("\n");  
//			}
		}
		else if (strcmp(key,requestId) == 0)//ȡ��requestId ��	
		{
				array_list* arr = json_object_get_array(val);
				json_object* obj = (json_object*)array_list_get_idx(arr,0);
				printf("requestId checking out\n");
				printf("requestId=%s\n", json_object_get_string(obj));
		}
	
		json_print_value(val);
	}
}

static int Process_json(json_object *new_obj)
{

	printf("new_obj.to_string()=%s\n", json_object_to_json_string(new_obj));
	printf("Result is %s\n", (new_obj == NULL) ? "NULL (error!)" : "not NULL");
	if (!new_obj)
		return 1; // oops, we failed.

	json_print_object(new_obj);//����json�����key��ֵ ����json����

	json_object_put(new_obj);/*�ͷ���Դ*/

}

/* lws-mirror_protocol �ص�����*/
static int
callback_DTS2B_mirror(struct libwebsocket_context *context,
			struct libwebsocket *wsi,
			enum libwebsocket_callback_reasons reason,
					       void *user, void *in, size_t len)
{
	
		
		int n;
		int num;
		json_object *get_obj;
	switch (reason) {

	//when the websocket session ends
	case LWS_CALLBACK_CLOSED:
		fprintf(stderr, "mirror: LWS_CALLBACK_CLOSED mirror_lifetime=%d\n", mirror_lifetime);
		wsi_mirror = NULL;
		was_closed = 1;
		break;
	//after your client connection completed a handshake with the remote server 
	case LWS_CALLBACK_CLIENT_ESTABLISHED:
	
		fprintf(stderr, "LWS_CALLBACK_CLIENT_ESTABLISHED\n");

		/*
		 * start the ball rolling,
		 * LWS_CALLBACK_CLIENT_WRITEABLE will come next service
		 */

		libwebsocket_callback_on_writable(context, wsi);
		break;
	//data has appeared from the server for the client connection, it can be found at *in and is len bytes long 
	case LWS_CALLBACK_CLIENT_RECEIVE://���շ������Ϣ
		((char *)in)[len] = '\0';
		fprintf(stderr, "Received %d '%s'\n", (int)len, (char *)in); //��ӡ���յ�������
		get_obj=json_tokener_parse((char *)in);//�����յ����ַ���ת��Ϊjson object
		Process_json(get_obj);//����������յ�������
		break;
	/*

	If you call libwebsocket_callback_on_writable on a connection, 
	you will get one of these callbacks coming when the connection socket is able to accept another write packet without blocking. 
	If it already was able to take another packet without blocking, 
	you'll get this callback at the next call to the service loop function. 
	Notice that CLIENTs get LWS_CALLBACK_CLIENT_WRITEABLE and servers get LWS_CALLBACK_SERVER_WRITEABLE.

	*/
	//������д����
	case LWS_CALLBACK_CLIENT_WRITEABLE:

		
		num=strlen(text_send_test());//�����ݳ���

		n = libwebsocket_write(wsi,text_send_test(), num,LWS_WRITE_TEXT);//�����ı�����
		//n = libwebsocket_write(wsi,json_text_test(), num,  LWS_WRITE_TEXT);//����JSON ��������
		//n = libwebsocket_write(wsi,reportDeviceLog(), num,LWS_WRITE_TEXT);//����DTS��־
		//n = libwebsocket_write(wsi,reportDeviceStatus(), num,LWS_WRITE_TEXT);//����DTS״̬
		//n = libwebsocket_write(wsi,reportDeviceRuntimeInfo(), num,LWS_WRITE_TEXT);//�ϱ�DTS������Ϣ
		//n = libwebsocket_write(wsi,requestActiveDtsDevice(), num,LWS_WRITE_TEXT);//����DTS�豸
		//n = libwebsocket_write(wsi,reportDtsRfidCheckOnData(), num,LWS_WRITE_TEXT);//�ϱ�RFID ˢ������
		//n = libwebsocket_write(wsi,RequestSyncDtsAllConfigOptions(), num,LWS_WRITE_TEXT);//����ͬ��DTS����
		//n = libwebsocket_write(wsi,RequestSyncDtsAllRfidPermission(), num,LWS_WRITE_TEXT);//����ͬ��RFIDȨ��
		
		printf("Send success %s\n", text_send_test());//��ӡ������Ϣ
		if (n < 0)
		return -1;

		/* get notified as soon as we can write again */
		libwebsocket_callback_on_writable(context, wsi);
		sleep(3);//ÿ��3�뷢��һ������
		//was_closed = 1;//�ر�websocketͨѶ
		break;

	default:
		break;
	}

	return 0;
}


/* list of supported protocols and callbacks */
/*
struct libwebsocket_protocols {
    const char * name;
    callback_function * callback;
    size_t per_session_data_size;
    size_t rx_buffer_size;
    struct libwebsocket_context * owning_server;
    int protocol_index;
};
name
    Protocol name that must match the one given in the client Javascript new WebSocket(url, 'protocol') name 
callback
    The service callback used for this protocol. It allows the service action for an entire protocol to be encapsulated in the protocol-specific callback 
���ڴ�Э��Ļص���������������װ��Э���ض��ص�����������Ϊ����Э��
per_session_data_size
    Each new connection using this protocol gets this much memory allocated on connection establishment and freed on connection takedown. A pointer to this per-connection allocation is passed into the callback in the 'user' parameter 

rx_buffer_size
    if you want atomic frames delivered to the callback, you should set this to the size of the biggest legal frame that you support. If the frame size is exceeded, there is no error, but the buffer will spill to the user callback when full, which you can detect by using libwebsockets_remaining_packet_payload. Notice that you just talk about frame size here, the LWS_SEND_BUFFER_PRE_PADDING and post-padding are automatically also allocated on top.
owning_server
    the server init call fills in this opaque pointer when registering this protocol with the server. 

protocol_index
    which protocol we are starting from zero 

Description

    This structure represents one protocol supported by the server. An array of these structures is passed to libwebsocket_create_server allows as many protocols as you like to be handled by one server. 
    

*/
static struct libwebsocket_protocols protocols[] = {
	{
		"lws-mirror-protocol",
		callback_DTS2B_mirror,
		0,
		0,
	},
	{ NULL, NULL, 0, 0 } /* end */
};




int main(int argc, char **argv)
{
	int n = 0;
	int ret = 0;
//	int port = 8543;
	int port = 9000;
	int use_ssl = 0;
	struct libwebsocket_context *context;
//	const char *address="192.168.6.176";
	const char *address="192.168.6.114";

	struct libwebsocket *wsi_dumb;
	int ietf_version = -1; /* latest */
	struct lws_context_creation_info info;

	memset(&info, 0, sizeof info);

	fprintf(stderr, "DTS2B websockets client\n"
			"(C) Copyright 2014-2015 Mleaf_HEXI <350983773@qq.com> "
						    "licensed under LGPL2.1\n");


	/*
	 * create the websockets context.  This tracks open connections and
	 * knows how to route any traffic and which protocol version to use,
	 * and if each connection is client or server side.
	 *
	 * For this client-only demo, we tell it to not listen on any port.
	 */

	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
#ifndef LWS_NO_EXTENSIONS
	info.extensions = libwebsocket_get_internal_extensions();
#endif
	info.gid = -1;
	info.uid = -1;

	context = libwebsocket_create_context(&info);
	if (context == NULL) {
		fprintf(stderr, "Creating libwebsocket context failed\n");
		return 1;
	}

	/* create a client websocket using dumb increment protocol */
	/*

TITLE:	libwebsocket_client_connect - Connect to another websocket server

	struct libwebsocket * libwebsocket_client_connect (struct libwebsocket_context * context, const char * address, int port, int ssl_connection, const char * path, const char * host, const char * origin, const char * protocol, int ietf_version_or_minus_one)

	Arguments

	context
	    Websocket context 
	address
	    Remote server address, eg, "myserver.com" 
	port
	    Port to connect to on the remote server, eg, 80 
	ssl_connection
	    0 = ws://, 1 = wss:// encrypted, 2 = wss:// allow self signed certs 
	path
	    Websocket path on server 
	host
	    Hostname on server 
	origin
	    Socket origin name 
	protocol
	    Comma-separated list of protocols being asked for from the server, or just one. The server will pick the one it likes best. 
	ietf_version_or_minus_one
	    -1 to ask to connect using the default, latest protocol supported, or the specific protocol ordinal 

	Description

	    This function creates a connection to a remote server 

	*/
	fprintf(stderr, "Connecting to %s:%u\n", address, port);
	wsi_dumb = libwebsocket_client_connect(context, address, port, use_ssl,
			"/websocket/uclient/MOCK_DTS2B_ACCESS_CODE_1234567890", address,"origin",
			 protocols[0].name, ietf_version);

	if (wsi_dumb == NULL) {
		fprintf(stderr, "libwebsocket connect failed\n");
		ret = 1;
		goto bail;
	}

	fprintf(stderr, "Waiting for connect...\n");
	sqlite3_test();//sqlite3���ݿ����

	/*
	 * sit there servicing the websocket context to handle incoming
	 * packets, and drawing random circles on the mirror protocol websocket
	 * nothing happens until the client websocket connection is
	 * asynchronously established
	 */
	n = 0;
	while (n >= 0 && !was_closed && !force_exit) 
		{
		n = libwebsocket_service(context, 10);

		if (n < 0)
			continue;

		if (wsi_mirror)
			continue;		
	}

bail:
	fprintf(stderr, "Exiting\n");

	libwebsocket_context_destroy(context);

	return 1;
}
