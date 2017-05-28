/*
 * fixpara.c
 *
 *  Created on: Feb 10, 2017
 *      Author: ava
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>
#include "sys/reboot.h"
#include <wait.h>
#include <errno.h>

#include "ParaDef.h"
#include "PublicFunction.h"
#include "AccessFun.h"
#include "Objectdef.h"

typedef struct {
    INT8U apn[VISIBLE_STRING_LEN];      // apn
    INT8U userName[VISIBLE_STRING_LEN]; //用户名称
    INT8U passWord[VISIBLE_STRING_LEN]; //密码
    INT8U proxyIp[OCTET_STRING_LEN];    //代理服务器地址
}GprsPara;

								//厂商代码　　软件版本　软件日期　　硬件版本　硬件日期  扩展信息
static VERINFO verinfo          = { "QDGK", "V1.2", "170328", "1.10", "150628", "00000000" }; // 4300 版本信息
static DateTimeBCD product_date = { { 2016 }, { 04 }, { 6 }, { 0 }, { 0 }, { 0 } };   // 4300 生产日期
static char protcol[]           = "DL/T 698.45";                                      // 4300 支持规约类型

///湖南　Ｉ型
static MASTER_STATION_INFO	master_info_HuNan = {{10,223,31,200},4000};			//IP				端口号
static GprsPara 	gprs_para_HuNan = {"dl.vpdn.hn","cs@dl.vpdn.hn","hn123456",""};		//apn ,userName,passWord,proxyIp

///浙江　ＩＩ型
static MASTER_STATION_INFO	master_info_ZheJiang = {{10,223,31,200},4000};		//IP				端口号
static GprsPara 	gprs_para_ZheJiang = {"ZJDL.ZJ","card.ZJ","card",""};		//apn ,userName,passWord,proxyIp

void InitClass4500(MASTER_STATION_INFO master_info,GprsPara gprs_para)
{
	int		i=0;
	CLASS25 class4500 = {};
    memset(&class4500, 0, sizeof(CLASS25));

    class4500.commconfig.workModel     = 1; //客户机模式
    class4500.commconfig.listenPortnum = 1;
    class4500.commconfig.heartBeat     = 60; // 60s
    memcpy(&class4500.commconfig.apn[1],&gprs_para.apn,strlen((char *)gprs_para.apn));
    class4500.commconfig.apn[0] = strlen((char *)gprs_para.apn);
    memcpy(&class4500.commconfig.userName[1],&gprs_para.userName,strlen((char *)gprs_para.userName));
    class4500.commconfig.userName[0] = strlen((char *)gprs_para.userName);
    memcpy(&class4500.commconfig.passWord[1],&gprs_para.passWord,strlen((char *)(gprs_para.passWord)));
    class4500.commconfig.passWord[0] = strlen((char *)(gprs_para.passWord));
    class4500.master.masternum = 2;
    for(i=0;i<class4500.master.masternum;i++) {
    	memcpy(&class4500.master.master[i].ip[1],&master_info.ip,strlen((char *)master_info.ip));
    	class4500.master.master[i].ip[0] = strlen((char *)master_info.ip);
    	class4500.master.master[i].port = master_info.port;
    }
    fprintf(stderr, "\n主IP %d.%d.%d.%d:%d  ", class4500.master.master[0].ip[1],
            class4500.master.master[0].ip[2], class4500.master.master[0].ip[3],
            class4500.master.master[0].ip[4], class4500.master.master[0].port);
    fprintf(stderr, "备IP %d.%d.%d.%d:%d\n", class4500.master.master[1].ip[1],
            class4500.master.master[1].ip[2], class4500.master.master[1].ip[3],
            class4500.master.master[1].ip[4], class4500.master.master[1].port);
    saveCoverClass(0x4500, 0, &class4500, sizeof(CLASS25), para_vari_save);
}

/*
 * type = 1: 判断参数不存在初始化
 * 　　　　　= 0: 初始化参数
 * */
void InitClassByZone(INT8U type)
{
	int ret        = 0;
	CLASS25 class4500 = {};

	if(type == 1) {
    	ret = readCoverClass(0x4500, 0, (void*)&class4500, sizeof(CLASS25), para_vari_save);
	}else ret = 0;
    if (ret != 1) {

    	if(getZone("ZheJiang")==0) {
			InitClass4500(master_info_ZheJiang,gprs_para_ZheJiang);
		}else if(getZone("HuNan")==0) {
			InitClass4500(master_info_HuNan,gprs_para_HuNan);
		}
    }
}

void InitClass4510() //以太网通信模块1
{
    CLASS26 oi4510 = {};
    int ret        = 0;
    memset(&oi4510, 0, sizeof(CLASS26));
    ret = readCoverClass(0x4510, 0, (void*)&oi4510, sizeof(CLASS26), para_vari_save);
    if (ret != 1) {
        fprintf(stderr, "\n初始化以太网通信模块1：4510\n");
        oi4510.commconfig.workModel     = 1; //客户机模式
        oi4510.commconfig.connectType   = 0;
        oi4510.commconfig.heartBeat     = 60; // 60s
        oi4510.master.masternum         = 1;
        oi4510.master.master[0].ip[1]   = 193;
        oi4510.master.master[0].ip[2]   = 168;
        oi4510.master.master[0].ip[3]   = 18;
        oi4510.master.master[0].ip[4]   = 70;
        oi4510.master.master[0].port    = 5022;
        saveCoverClass(0x4510, 0, &oi4510, sizeof(CLASS26), para_vari_save);
    }
}

/*
 * 初始化当前套日时段表
 * */
void InitClass4016() {
    CLASS_4016 class4016 = {}; //当前套日时段表
    int readret          = 0;
    INT8U i              = 0;

    memset(&class4016, 0, sizeof(CLASS_4016));
    readret = readCoverClass(0x4016, 0, &class4016, sizeof(CLASS_4016), para_vari_save);
    if (readret != 1) {
    	fprintf(stderr, "\n初始化当前套日时段表：4016\n");
        class4016.num = MAX_PERIOD_RATE / 2;
        for (i = 0; i < class4016.num; i++) {
            class4016.Period_Rate[i].hour   = i;
            class4016.Period_Rate[i].min    = 0;
            class4016.Period_Rate[i].rateno = (i % 4) + 1;
        }
        saveCoverClass(0x4016, 0, &class4016, sizeof(CLASS_4016), para_vari_save);
    }
}

void InitClass4300() //电气设备信息
{
    CLASS19 oi4300 = {};
    int ret        = 0;

    memset(&oi4300, 0, sizeof(CLASS19));
    ret = readCoverClass(0x4300, 0, &oi4300, sizeof(CLASS19), para_vari_save);
    if ((ret != 1) || (memcmp(&oi4300.info, &verinfo, sizeof(VERINFO)) != 0)
    				|| memcmp(&oi4300.date_Product, &product_date, sizeof(DateTimeBCD)) != 0
    				|| memcmp(&oi4300.protcol, protcol, sizeof(protcol)) != 0) {
        fprintf(stderr, "\n初始化电气设备信息：4300\n");
        memcpy(&oi4300.info, &verinfo, sizeof(VERINFO));
        memcpy(&oi4300.date_Product, &product_date, sizeof(DateTimeBCD));
        memcpy(&oi4300.protcol, protcol, sizeof(oi4300.protcol));
        saveCoverClass(0x4300, 0, &oi4300, sizeof(CLASS19), para_vari_save);
    }
    fprintf(stderr, "\n厂商代码 %c%c%c%c", oi4300.info.factoryCode[0], oi4300.info.factoryCode[1], oi4300.info.factoryCode[2], oi4300.info.factoryCode[3]);
    fprintf(stderr, "\n软件版本 %c%c%c%c", oi4300.info.softVer[0], oi4300.info.softVer[1], oi4300.info.softVer[2], oi4300.info.softVer[3]);
    fprintf(stderr, "\n软件版本日期 %c%c%c%c%c%c", oi4300.info.softDate[0], oi4300.info.softDate[1], oi4300.info.softDate[2], oi4300.info.softDate[3],
            oi4300.info.softDate[4], oi4300.info.softDate[5]);
    fprintf(stderr, "\n硬件版本 %c%c%c%c", oi4300.info.hardVer[0], oi4300.info.hardVer[1], oi4300.info.hardVer[2], oi4300.info.hardVer[3]);
    fprintf(stderr, "\n硬件版本日期 %c%c%c%c%c%c", oi4300.info.hardDate[0], oi4300.info.hardDate[1], oi4300.info.hardDate[2], oi4300.info.hardDate[3],
            oi4300.info.hardDate[4], oi4300.info.hardDate[5]);
    fprintf(stderr, "\n规约列表 %s", oi4300.protcol);
    fprintf(stderr, "\n生产日期 %d-%d-%d\n", oi4300.date_Product.year.data, oi4300.date_Product.month.data, oi4300.date_Product.day.data);
}

/*
 * 初始化采集配置单元
 * */
void InitClass6000() {
    CLASS_6001 meter        = {};
    int readret             = 0;
    static INT8U ACS_TSA[8] = { 0x08, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }; //[0]=08,TSA长度 [1]=05,05+1=地址长度，交采地址：000000000001

    readret = readParaClass(0x6000, &meter, 1);
    if (readret != 1) {
        fprintf(stderr, "\n初始化采集配置单元：6000\n");
        memset(&meter, 0, sizeof(CLASS_6001));
        meter.sernum = 1;
        memcpy(meter.basicinfo.addr.addr, ACS_TSA, sizeof(ACS_TSA));
        meter.basicinfo.baud           = 3;
        meter.basicinfo.protocol       = 3;
        meter.basicinfo.port.OI        = 0xF208;
        meter.basicinfo.port.attflg    = 0x02;
        meter.basicinfo.port.attrindex = 0x01;
        meter.basicinfo.ratenum        = MAXVAL_RATENUM;
        //		meter.basicinfo.connectype = getACSConnectype();
        meter.basicinfo.ratedU = 2200;
        meter.basicinfo.ratedI = 1500;
        saveParaClass(0x6000, &meter, meter.sernum);
    }
}

/*
 * 开关量输入
 * */
void InitClassf203() {
    CLASS_f203 oif203 = {};
    int readret       = 0;

    memset(&oif203, 0, sizeof(oif203));
    readret = readCoverClass(0xf203, 0, &oif203, sizeof(CLASS_f203), para_vari_save);
    if (readret != 1) {
        fprintf(stderr, "初始化开关量输入：【F203】\n");
        strncpy((char*)&oif203.class22.logic_name, "F203", sizeof(oif203.class22.logic_name));
        oif203.class22.device_num    = 1;
        oif203.statearri.num         = 4;
        oif203.state4.StateAcessFlag = 0xF0; //第1路状态接入
        oif203.state4.StatePropFlag  = 0xF0; //第1路状态常开触点
        saveCoverClass(0xf203, 0, &oif203, sizeof(CLASS_f203), para_vari_save);
    }
}
