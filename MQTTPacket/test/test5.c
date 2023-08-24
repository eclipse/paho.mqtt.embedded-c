/*******************************************************************************
 * Copyright (c) 2023 Microsoft Corporation. All rights reserved.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 *******************************************************************************/

/***
  Side-by-side MQTTv3 and MQTTv5 Tests.
***/

#include "test_framework.h"
#include "MQTTPacket.h"
#include "MQTTV5Packet.h"
#include "transport.h"

#include "test_mqtt3.h"
#include "test_mqtt5.h"

int main(int argc, char** argv)
{
	int rc = 0;
 	int (*tests[])(struct Options) = {NULL, test_v3, test_v5};

	xml = fopen("TEST-test5.xml", "w");
	fprintf(xml, "<testsuite name=\"test35\" tests=\"%d\">\n", (int)(ARRAY_SIZE(tests) - 1));

	getopts(argc, argv);

 	if (options.test_no == 0)
	{ /* run all the tests */
 	   	for (options.test_no = 1; options.test_no < ARRAY_SIZE(tests); ++options.test_no)
			rc += tests[options.test_no](options); /* return number of failures.  0 = test succeeded */
	}
	else
 	   	rc = tests[options.test_no](options); /* run just the selected test */

 	if (rc == 0)
		MyLog(LOGA_INFO, "verdict pass");
	else
		MyLog(LOGA_INFO, "verdict fail");

	fprintf(xml, "</testsuite>\n");
	fclose(xml);
	return rc;
}
