/*
 * Copyright (c) 2014, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define STR_TRACE_USER_TA "HELLO_WORLD"

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <tee_internal_se_api.h>

#include "ta_se_api_test.h"
#include "util.h"


/*
 * Called when the instance of the TA is created. This is the first call in
 * the TA.
 */
TEE_Result TA_CreateEntryPoint(void)
{
	return TEE_SUCCESS;
}

/*
 * Called when the instance of the TA is destroyed if the TA has not
 * crashed or panicked. This is the last call in the TA.
 */
void TA_DestroyEntryPoint(void)
{
}

/*
 * Called when a new session is opened to the TA. *sess_ctx can be updated
 * with a value to be able to identify this session in subsequent calls to the
 * TA.
 */
TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
		TEE_Param  params[4], void **sess_ctx)
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Unused parameters */
	(void)&params;
	(void)&sess_ctx;

	/*
	 * The DMSG() macro is non-standard, TEE Internal API doesn't
	 * specify any means to logging from a TA.
	 */
	DMSG("Hello World!\n");

	/* If return value != TEE_SUCCESS the session will not be created. */
	return TEE_SUCCESS;
}

/*
 * Called when a session is closed, sess_ctx hold the value that was
 * assigned by TA_OpenSessionEntryPoint().
 */
void TA_CloseSessionEntryPoint(void *sess_ctx)
{
	(void)&sess_ctx; /* Unused parameter */
	DMSG("Goodbye!\n");
}

#define CFG_MAX_READER_NUM	10

#define READER_NAME_BUF_LEN	20
#define RESP_BUF_LEN		20
#define ATR_BUF_LEN		50

static TEE_Result Test_SE_API(void)
{
	TEE_Result ret;
	TEE_SEServiceHandle service;
	TEE_SEReaderHandle readers[CFG_MAX_READER_NUM];
	TEE_SESessionHandle session1, session2;
	TEE_SEChannelHandle basic_channel, logical_channel;
	TEE_SEAID aid;
	size_t i, numReaders = CFG_MAX_READER_NUM;
	uint8_t aidBuffer[] =
		{ 0xD0, 0x00, 0x0C, 0xAF, 0xE0, 0x00 };
	uint8_t commandBuffer[] = { 0x00, 0x01, 0x00, 0x00 };
	uint8_t responseBuffer[RESP_BUF_LEN];
	size_t responseBufferLen = RESP_BUF_LEN;
	uint8_t atrBuf[ATR_BUF_LEN];
	size_t atrBufLen = ATR_BUF_LEN;

	ret = TEE_SEServiceOpen(&service);
	if (ret != TEE_SUCCESS)
		return ret;

	ret = TEE_SEServiceGetReaders(service, readers, &numReaders);
	if (ret != TEE_SUCCESS)
		return ret;

	for (i = 0; i < numReaders; i++) {
		char name[READER_NAME_BUF_LEN];
		size_t name_len = sizeof(name);
		TEE_SEReaderProperties prop;

		TEE_SEReaderGetName(readers[i], name, &name_len);
		TEE_SEReaderGetProperties(readers[i], &prop);

		name[name_len] = '\0';
		DMSG("reader %d: %s", i, name);
		DMSG(" SE Present: %s", prop.sePresent ? "true" : "false");
	}

	ret = TEE_SEReaderOpenSession(readers[0], &session1);
	if (ret != TEE_SUCCESS)
		return ret;

	ret = TEE_SEReaderOpenSession(readers[0], &session2);
	if (ret != TEE_SUCCESS)
		return ret;

	ret = TEE_SESessionGetATR(session1, atrBuf, &atrBufLen);
	if (ret != TEE_SUCCESS)
		return ret;
	DMSG("ATR dump:");
	print_hex(atrBuf, atrBufLen);

	aid.buffer = aidBuffer;
	aid.bufferLen = sizeof(aidBuffer);

	/*
	 * Test Basic Channel
	 */
	ret = TEE_SESessionOpenBasicChannel(session1, &aid, &basic_channel);
	if (ret != TEE_SUCCESS)
		return ret;

	responseBufferLen = RESP_BUF_LEN;
	ret = TEE_SEChannelGetSelectResponse(basic_channel,
			responseBuffer, &responseBufferLen);
	DMSG("SELECT response:");
	print_hex(responseBuffer, responseBufferLen);

	responseBufferLen = RESP_BUF_LEN;
	ret = TEE_SEChannelTransmit(basic_channel,
			commandBuffer, sizeof(commandBuffer),
			responseBuffer, &responseBufferLen);
	if (ret != TEE_SUCCESS)
		return ret;

	DMSG("Basic channel response:");
	print_hex(responseBuffer, responseBufferLen);


	/*
	 * Test Logical Channel & Select Next
	 */
	ret = TEE_SESessionOpenLogicalChannel(session2, &aid, &logical_channel);
	if (ret != TEE_SUCCESS)
		return ret;

	responseBufferLen = RESP_BUF_LEN;
	ret = TEE_SEChannelTransmit(logical_channel,
			commandBuffer, sizeof(commandBuffer),
			responseBuffer, &responseBufferLen);
	if (ret != TEE_SUCCESS)
		return ret;

	DMSG("Logical channel response:");
	print_hex(responseBuffer, responseBufferLen);

	ret = TEE_SEChannelSelectNext(logical_channel);
	if (ret != TEE_SUCCESS)
		return ret;

	responseBufferLen = RESP_BUF_LEN;
	ret = TEE_SEChannelGetSelectResponse(logical_channel,
			responseBuffer, &responseBufferLen);
	DMSG("SELECT_NEXT response:");
	print_hex(responseBuffer, responseBufferLen);

	responseBufferLen = RESP_BUF_LEN;
	ret = TEE_SEChannelTransmit(logical_channel,
			commandBuffer, sizeof(commandBuffer),
			responseBuffer, &responseBufferLen);
	if (ret != TEE_SUCCESS)
		return ret;

	DMSG("Logical channel response: (after SELECT_NEXT)");
	print_hex(responseBuffer, responseBufferLen);

	/*
	 * Test close session
	 */
	TEE_SEReaderCloseSessions(readers[0]);

	ret = TEE_SESessionIsClosed(session1);
	if (ret != TEE_SUCCESS)
		return ret;

	ret = TEE_SESessionIsClosed(session2);
	if (ret != TEE_SUCCESS)
		return ret;

	TEE_SEServiceClose(service);

	return TEE_SUCCESS;
}

/*
 * Called when a TA is invoked. sess_ctx hold that value that was
 * assigned by TA_OpenSessionEntryPoint(). The rest of the paramters
 * comes from normal world.
 */
TEE_Result TA_InvokeCommandEntryPoint(void *sess_ctx, uint32_t cmd_id,
			uint32_t param_types, TEE_Param params[4])
{
	(void)&sess_ctx; /* Unused parameter */
	(void)&param_types;
	(void)&params;

	switch (cmd_id) {
	case CMD_SELF_TESTS:
		return Test_SE_API();
	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}
}
