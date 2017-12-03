/*
* Copyright (c) 2012-2014  Kirill Belyaev
* kirillbelyaev@yahoo.com
* kirill@cs.colostate.edu
* TeleScope - XML Message Stream Broker/Replicator Platform
* This work is licensed under the Creative Commons Attribution-NonCommercial 3.0 Unported License.
* To view a copy of this license, visit http://creativecommons.org/licenses/by-nc/3.0/ or send
* a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
*
* Author: Vignesh M. Pagadala
* Vignesh.Pagadala@colostate.edu
* Last update: December 2nd, 2017 
*
* File: Control_Message_Processor.C
* ----------------------------------
* Contains functions for parsing an XML control message, filtering out the Upstream Brokers List 
* and Filtering Query from the message and performing validation using Secure Hash Algorithm 1.
*/

#include "Control_Message_Processor.h"

char *Control_Message_Processor_CONT;
uint_fast8_t Control_Message_Processor_Flag = 0;
char *Control_Message_Processor_UBL;
char *Control_Message_Processor_FQ;
char *Control_Message_Processor_SHA1;

void delay(unsigned int seconds)
{
	int elapsed = time(0) + seconds;
	while (time(0) < elapsed);
}

// Expat handler for detecting element start tags.
void start(void *data, const char *element, const char **Attribute)
{
	if (Control_Message_Processor_Flag == 1)
	{
		strcat(Control_Message_Processor_UBL, element);
		strcat(Control_Message_Processor_UBL, ":");

		if (Attribute[0] != NULL)
		{
			uint_fast8_t i;
			for (i = 0; Attribute[i]; i += 2)
			{
				strcat(Control_Message_Processor_UBL, Attribute[i]);
				strcat(Control_Message_Processor_UBL, "=");
				strcat(Control_Message_Processor_UBL, Attribute[i + 1]);
				strcat(Control_Message_Processor_UBL, ",");
			}
		}
	}
	if (strcmp(element, "UBL") == 0)
	{
		Control_Message_Processor_Flag = 1;
	}
	if (strcmp(element, "FilteringQuery") == 0)
	{
		Control_Message_Processor_Flag = 2;
	}
	if (strcmp(element, "SHA1") == 0)
	{
		Control_Message_Processor_Flag = 3;
	}
	if (strcmp(element, "ControlMessageLength") == 0)
	{
		Control_Message_Processor_Flag = 4;
	}
}

// Expat handler for detecting element end tags.
void end(void* data, const char* element)
{
	if (strcmp(element, "ControlMessageLength") == 0 && Control_Message_Processor_Flag == 4)
	{
		Control_Message_Processor_UBL = (char*)malloc(sizeof(char*) * atoi(Control_Message_Processor_CONT));
		if (Control_Message_Processor_UBL == NULL)
		{
			perror("Error: Could not allocate memory for Upstream Brokers List buffer. \n Terminating program...");
			delay(1);
			exit(0);
		}
		*Control_Message_Processor_UBL = '\0';
		Control_Message_Processor_Flag = 0;
	}

	// For UBL
	if (Control_Message_Processor_Flag == 1)
	{
		if (strcmp(element, "UBL") == 0)
		{
			Control_Message_Processor_Flag = 0;
		}

		if (strcmp(element, "Broker1") == 0 || strcmp(element, "Broker2") == 0 || strcmp(element, "Broker3") == 0 || strcmp(element, "Broker4") == 0)
		{
			strcat(Control_Message_Processor_UBL, "\n");
		}
	}

	// For FQ.
	if (Control_Message_Processor_Flag == 2)
	{
		Control_Message_Processor_FQ = (char*)malloc(sizeof(char) * (strlen(Control_Message_Processor_CONT) + 1));
		if (Control_Message_Processor_FQ == NULL)
		{
			perror("Error: Could not allocate memory for Filtering Query buffer. \n Terminating program...");
			delay(1);
			exit(0);
		}
		*Control_Message_Processor_FQ = '\0';
		strcat(Control_Message_Processor_FQ, Control_Message_Processor_CONT);
		if (strcmp(element, "FilteringQuery") == 0)
		{
			Control_Message_Processor_Flag = 0;
		}
	}

	// For SHA-1.
	if (Control_Message_Processor_Flag == 3)
	{
		Control_Message_Processor_SHA1 = (char*)malloc(sizeof(char) * (strlen(Control_Message_Processor_CONT) + 1));
		if (Control_Message_Processor_SHA1 == NULL)
		{
			perror("Error: Could not allocate memory for SHA-1 digest buffer. \n Terminating program...");
			delay(1);
			exit(0);
		}
		*Control_Message_Processor_SHA1 = '\0';
		strcat(Control_Message_Processor_SHA1, Control_Message_Processor_CONT);
		if (strcmp(element, "SHA") == 0)
		{
			Control_Message_Processor_Flag = 0;
		}
	}
}

// Expat handler for getting contents.
void content(void *data, const char *content, int length)
{
	char *temp = (char*)malloc(sizeof(char*) * length);
	if (temp == NULL)
	{
		perror("Error: Could not allocate memory for content buffer. \n Terminating program...");
		delay(1);
		exit(0);
	}
	strncpy(temp, content, length);
	temp[length] = '\0';
	Control_Message_Processor_CONT = temp;
}

char* UBL()
{
	return Control_Message_Processor_UBL;
}

char* FQ()
{
	return Control_Message_Processor_FQ;
}

char* shafunc(char *buff)
{
	/*
	* 1. Create copy of buff in testbuf.
	* 2. Process testbuf to strip the existing <SHA1> element contents.
	* 3. Store this processed testbuf in testbuf2.
	* 4. Use testbuf2 to compute digest and return it.
	*/

	uint_fast16_t Buffer_Length = strlen(buff) + 1;

	char *testbuf = (char*)malloc(sizeof(char*) * Buffer_Length);
	if (testbuf == NULL)
	{
		perror("Error: Could not allocate memory for testbuf. \n Terminating program...");
		delay(1);
		exit(0);
	}
	strcpy(testbuf, buff);

	uint_fast8_t shaflag = 0;
	uint_fast16_t kc = 0;

	char *testbuf2 = (char*)malloc(sizeof(char*) * Buffer_Length);
	if (testbuf2 == NULL)
	{
		perror("Error: Could not allocate memory for testbuf2. \n Terminating program...");
		delay(1);
		exit(0);
	}

	char shatag[7];
	for (uint_fast16_t l = 0; testbuf[l] != '\0'; l++)
	{
		for (uint_fast16_t k = 0; k < 5; k++)
		{
			shatag[k] = shatag[k + 1];
		}
		shatag[5] = testbuf[l];
		shatag[6] = '\0';
		if (strcmp(shatag, "<SHA1>") == 0)
		{
			testbuf2[kc] = testbuf[l];
			kc++;
			shaflag = 1;
		}

		if (strcmp(shatag, "</SHA1") == 0)
		{
			testbuf2[kc] = '<';
			kc++;
			testbuf2[kc] = '/';
			kc++;
			testbuf2[kc] = 'S';
			kc++;
			testbuf2[kc] = 'H';
			kc++;
			testbuf2[kc] = 'A';
			kc++;
			shaflag = 0;
		}
		if (shaflag == 0)
		{
			testbuf2[kc] = testbuf[l];
			kc++;
		}
	}
	testbuf2[kc] = '\0';
	free(testbuf);

	const char* testhash = (const char*)testbuf2;
	unsigned char *hash = (unsigned char*)malloc(sizeof(char*) * (SHA_DIGEST_LENGTH * 2 + 1));
	if (hash == NULL)
	{
		perror("Error: Could not allocate memory for hash. \n Terminating program...");
		delay(1);
		exit(0);
	}

	SHA_CTX context;
	SHA1_Init(&context);
	SHA1_Update(&context, testhash, strlen(testhash));
	SHA1_Final(hash, &context);

	char *digest = (char*)malloc(sizeof(char*)*(SHA_DIGEST_LENGTH * 2 + 1));
	if (digest == NULL)
	{
		perror("Error: Could not allocate memory for digest. \n Terminating program...");
		delay(1);
		exit(0);
	}
	for (uint_fast8_t i = 0; i < SHA_DIGEST_LENGTH; i++)
		sprintf(&digest[i * 2], "%02x", (unsigned int)hash[i]);
	free(testbuf2);
	free(hash);

	return digest;
}

int_fast8_t filter(char *buff)
{
	// If any element is missing, return -1.
	if (strstr(buff, "<CRL_MESSAGE length") == NULL || strstr(buff, "<ControlMessageLength>") == NULL || strstr(buff, "<ControlMessageID>") == NULL || strstr(buff, "<RootBrokerID>") == NULL || strstr(buff, "<ChildBrokerID>") == NULL || strstr(buff, "<UBL>") == NULL || strstr(buff, "<FilteringQuery>") == NULL || strstr(buff, "<SHA1>") == NULL)
	{
		return -1;
	}
	// If any element (except UBL and FQ) is empty, return -1.
	if (strstr(buff, "<ControlMessageLength></ControlMessageLength>") != NULL || strstr(buff, "<ControlMessageID></ControlMessageID>") != NULL || strstr(buff, "<RootBrokerID></RootBrokerID>") != NULL || strstr(buff, "<ChildBrokerID></ChildBrokerID>") != NULL || strstr(buff, "<FilteringQuery></FilteringQuery>") != NULL || strstr(buff, "<SHA1></SHA1>") != NULL)
	{
		return -1;
	}

	// Initiate parse.
	XML_Parser p = XML_ParserCreate(NULL);
	XML_SetElementHandler(p, start, end);
	XML_SetCharacterDataHandler(p, content);

	// If XML message isn't well-formed, return -1.
	if (XML_Parse(p, buff, strlen(buff), XML_TRUE) == XML_STATUS_ERROR)
	{
		return -1;
	}
	XML_ParserFree(p);

	char* digest = shafunc(buff);

	if (strcmp(Control_Message_Processor_SHA1, digest) == 0)
	{
		//printf("Upstream Brokers List: \n%s", UBL());
		//printf("\nFiltering Query: %s \n", FQ());
		free(digest);
		free(Control_Message_Processor_SHA1);
		free(Control_Message_Processor_FQ);
		free(Control_Message_Processor_UBL);
		free(Control_Message_Processor_CONT);
		return 0;
	}
	else
	{
		free(digest);
		free(Control_Message_Processor_SHA1);
		free(Control_Message_Processor_FQ);
		free(Control_Message_Processor_UBL);
		free(Control_Message_Processor_CONT);
		return -1;
	}
}

int process(char *buff)
{
	int_fast8_t isValid;
	isValid		= filter(buff);
	if (isValid == 0)
	{
		printf("VALID. filter() returned %d.", isValid);
	}
	else
	{
		printf("INVALID. filter() returned %d.", isValid);
	}
	return 0;
}