/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *
 * Contact: Jin Yoon <jinny.yoon@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <tizen.h>
#include <service_app.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <Ecore.h>

#include "log.h"
#include "resource.h"
#include "thingspark.h"

#define I2C_BUS_NUMBER (1)
#define SENSOR_GATHER_INTERVAL (30.1f)

typedef struct app_data_s {
	Ecore_Timer *getter_illuminance;
} app_data;

static app_data *g_ad = NULL;

#define ILLUMINATION_CRITERIA 1000

static Eina_Bool __get_illuminance_cb(void *data)
{
	int ret = 0;
	uint32_t value = 0;

	ret = resource_read_illuminance_sensor(I2C_BUS_NUMBER, &value);
	retv_if(ret != 0, ECORE_CALLBACK_RENEW);

	_D("Illuminance value : %u", value);

	// thingspark 연동 시작
	int ret1 = 0;
	tp_handle_h handle = NULL;

	// thingspark.kr 에서 채널별로 부여되는 api-key를 입력합니다.
	ret1 = tp_initialize("Kf66bWlzhRJL9gOt", &handle);
	retv_if(ret1 != 0, -1);

	char s1[10];
	sprintf(s1, "%u", value);

	// 필드 값 설정
	ret1 = tp_set_field_value(handle, 1, s1);
	retv_if(ret1 != 0, -1);

	// 서버로 데이타 전송
	ret1 = tp_send_data(handle);
	retv_if(ret1 != 0, -1);

	// thingspark handle 마무리
	ret1 = tp_finalize(handle);
	retv_if(ret1 != 0, -1);

	return ECORE_CALLBACK_RENEW;
}

void gathering_stop(void *data)
{
	app_data *ad = data;

	ret_if(!ad);

	if (ad->getter_illuminance) {
		ecore_timer_del(ad->getter_illuminance);
		ad->getter_illuminance = NULL;
	}
}

void gathering_start(void *data)
{
	app_data *ad = data;

	ret_if(!ad);

	gathering_stop(ad);

	ad->getter_illuminance = ecore_timer_add(SENSOR_GATHER_INTERVAL, __get_illuminance_cb, NULL);
	if (!ad->getter_illuminance)
		_E("Failed to add getter_illuminance");
}

static bool service_app_create(void *user_data)
{
	return true;
}

static void service_app_control(app_control_h app_control, void *user_data)
{
	gathering_start(user_data);
}

static void service_app_terminate(void *user_data)
{
	resource_close_illuminance_sensor();
	gathering_stop(user_data);
}

int main(int argc, char *argv[])
{
	app_data *ad = NULL;
	service_app_lifecycle_callback_s event_callback;

	ad = calloc(1, sizeof(app_data));
	retv_if(!ad, -1);

	g_ad = ad;

	event_callback.create = service_app_create;
	event_callback.terminate = service_app_terminate;
	event_callback.app_control = service_app_control;

	return service_app_main(argc, argv, &event_callback, ad);
}
