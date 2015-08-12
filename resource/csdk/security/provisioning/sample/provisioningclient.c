/******************************************************************
*
* Copyright 2015 Samsung Electronics All Rights Reserved.
*
*
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
******************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "logger.h"
#include "oic_malloc.h"
#include "utlist.h"
#include "ocprovisioningmanager.h"
#include "secureresourceprovider.h"
#include "oxmjustworks.h"
// TODO: PIN based OxM implementation is required.
//#include "oxmrandompin.h"
#include "oic_string.h"

#define MAX_URI_LENGTH (64)
#define MAX_PERMISSION_LENGTH (5)
#define CREATE (1)
#define READ (2)
#define UPDATE (4)
#define DELETE (8)
#define NOTIFY (16)
#define DASH '-'
#define PREDEFINED_TIMEOUT (10)
#define MAX_OWNED_DEVICE (10)
#define TAG  "provisioningclient"

static char CRED_FILE[] = "oic_svr_db_client.json";
static OicSecAcl_t        *gAcl1 = NULL;
static OicSecAcl_t        *gAcl2 = NULL;
static int gOwnershipState = 0;

typedef enum
{
    ownershipDone = 1 << 1,
    finalizeDone = 1 << 2,
    provisionCredDone = 1 << 3,
    provisionAclDone = 1 << 4
} StateManager;


/**
 * Perform cleanup for ACL
 * @param[in]    ACL
 */
static void deleteACL(OicSecAcl_t *acl)
{
    if (acl)
    {
        /* Clean Resources */
        for (int i = 0; i < (acl)->resourcesLen; i++)
        {
            OICFree((acl)->resources[i]);
        }
        OICFree((acl)->resources);

        /* Clean Owners */
        OICFree((acl)->owners);

        /* Clean ACL node itself */
        OICFree((acl));

        acl = NULL;
    }
}

/**
 * Calculate ACL permission from string to bit
 *
 * @param[in] temp_psm    Input data of ACL permission string
 * @param[in,out] pms    The pointer of ACL permission value
 * @return  0 on success otherwise -1.
 */
static int CalculateAclPermission(const char *temp_pms, uint16_t *pms)
{
    int i = 0;

    if (NULL == temp_pms || NULL == pms)
    {
        return -1;
    }
    *pms = 0;
    while (temp_pms[i] != '\0')
    {
        switch (temp_pms[i])
        {
            case 'C':
                {
                    *pms += CREATE;
                    i++;
                    break;
                }
            case 'R':
                {
                    *pms += READ;
                    i++;
                    break;
                }
            case 'U':
                {
                    *pms += UPDATE;
                    i++;
                    break;
                }
            case 'D':
                {
                    *pms += DELETE;
                    i++;
                    break;
                }
            case 'N':
                {
                    *pms += NOTIFY;
                    i++;
                    break;
                }
            case '_':
                {
                    i++;
                    break;
                }
            default:
                {
                    return -1;
                }
        }
    }
    return 0;
}

/**
 * Get the ACL property from user
 *
 * @param[in]    ACL Datastructure to save user inputs
 * @return  0 on success otherwise -1.
 */
static int InputACL(OicSecAcl_t *acl)
{
    int unused __attribute__((unused));
    char temp_id [UUID_LENGTH + 4] = {0,};
    char temp_rsc[MAX_URI_LENGTH + 1] = {0,};
    char temp_pms[MAX_PERMISSION_LENGTH + 1] = {0,};
    printf("******************************************************************************\n");
    printf("-Set ACL policy for target device\n");
    printf("******************************************************************************\n");
    //Set Subject.
    printf("-URN identifying the subject\n");
    printf("ex) 1111-1111-1111-1111 (16 Numbers except to '-')\n");
    printf("Subject : ");
    unused = scanf("%19s", temp_id);
    int j = 0;
    for (int i = 0; temp_id[i] != '\0'; i++)
    {
        if (DASH != temp_id[i])
            acl->subject.id[j++] = temp_id[i];
    }

    //Set Resource.
    printf("Num. of Resource : ");
    unused = scanf("%zu", &acl->resourcesLen);
    printf("-URI of resource\n");
    printf("ex)/oic/sh/temp/0 (Max_URI_Length: 64 Byte )\n");
    acl->resources = (char **)OICMalloc(acl->resourcesLen * sizeof(char *));
    if (NULL == acl->resources)
    {
        OC_LOG(ERROR, TAG, "Error while memory allocation");
        return -1;
    }
    for (int i = 0; i < acl->resourcesLen; i++)
    {
        printf("[%d]Resource : ", i + 1);
        unused = scanf("%64s", temp_rsc);
        acl->resources[i] = OICStrdup(temp_rsc);

        if (NULL == acl->resources[i])
        {
            OC_LOG(ERROR, TAG, "Error while memory allocation");
            return -1;
        }
        strncpy(acl->resources[i], temp_rsc, strlen(temp_rsc));
        acl->resources[i][strlen(temp_rsc)] = '\0';
    }
    // Set Permission
    do
    {
        printf("-Set the permission(C,R,U,D,N)\n");
        printf("ex) CRUDN, CRU_N,..(5 Charaters)\n");
        printf("Permission : ");
        unused = scanf("%5s", temp_pms);
    }
    while (0 != CalculateAclPermission(temp_pms, &(acl->permission)) );
    // Set Rowner
    printf("Num. of Rowner : ");
    unused = scanf("%zu", &acl->ownersLen);
    printf("-URN identifying the rowner\n");
    printf("ex) 1111-1111-1111-1111 (16 Numbers except to '-')\n");
    acl->owners = (OicUuid_t *)OICCalloc(acl->ownersLen, sizeof(OicUuid_t));
    if (NULL == acl->owners)
    {
        OC_LOG(ERROR, TAG, "Error while memory allocation");
        return -1;
    }
    for (int i = 0; i < acl->ownersLen; i++)
    {
        printf("[%d]Rowner : ", i + 1);
        unused = scanf("%19s", temp_id);
        j = 0;
        for (int k = 0; temp_id[k] != '\0'; k++)
        {
            if (DASH != temp_id[k])
            {
                acl->owners[i].id[j++] = temp_id[k];
            }
        }
    }
    return 0;
}


//FILE *client_fopen(const char *path, const char *mode)
FILE *client_fopen(const char *path, const char *mode)
{
    return fopen(CRED_FILE, mode);
}

void ProvisionCredCB(void* ctx, int nOfRes, OCProvisionResult_t *arr, bool hasError)
{
    if(!hasError)
    {
        OC_LOG(INFO, TAG, "Provision Credential IS DONE~~!!");
        gOwnershipState = 1;
    }
}

void ProvisionAclCB(void* ctx, int nOfRes, OCProvisionResult_t *arr, bool hasError)
{
    if(!hasError)
    {
        OC_LOG(INFO, TAG, "Provision Acl IS DONE~~!!");
        gOwnershipState = 1;
    }
}

void ProvisionPairwiseCB(void* ctx, int nOfRes, OCProvisionResult_t *arr, bool hasError)
{
    if(!hasError)
    {
        OC_LOG_V(INFO, TAG, "Context is %s", (char*)ctx);
        OC_LOG(INFO, TAG, "Provision Pairwise IS DONE~~!!");
        gOwnershipState = 1;
    }
}

void OwnershipTransferCB(void* ctx, int nOfRes, OCProvisionResult_t *arr, bool hasError)
{
    if(!hasError)
    {
        OC_LOG_V(INFO, TAG, "Context is %s", (char*)ctx);
        OC_LOG(INFO, TAG, "OWNERSHIP TRANSFER IS DONE~~!!");
        gOwnershipState = 1;
    }
}

void InputPinCB(char* pinBuf, size_t bufSize)
{
    if(pinBuf)
    {
        scanf("%s", pinBuf);
        pinBuf[bufSize - 1] = '\0';
    }
}

/**
 * Provisioning client sample using ProvisioningAPI
 */
int main(int argc, char **argv)
{
    OCStackResult res = OC_STACK_OK;
    int unused __attribute__((unused));

    gAcl1 = (OicSecAcl_t *)OICMalloc(sizeof(OicSecAcl_t));
    if (NULL == gAcl1)
    {
        OC_LOG(ERROR, TAG, "Error while memory allocation");
        goto error;
    }

    // Initialize Persistent Storage for SVR database
    OCPersistentStorage ps = {};
    ps.open = client_fopen;
    ps.read = fread;
    ps.write = fwrite;
    ps.close = fclose;
    ps.unlink = unlink;
    OCRegisterPersistentStorageHandler(&ps);

    if (OC_STACK_OK != OCInit(NULL, 0, OC_CLIENT_SERVER))
    {
        OC_LOG(ERROR, TAG, "OCStack init error");
        goto error;
    }

    OCProvisionDev_t* pDeviceList = NULL;
    res = OCDiscoverUnownedDevices(PREDEFINED_TIMEOUT, &pDeviceList);
    if(OC_STACK_OK != res)
    {
        OC_LOG_V(ERROR, TAG, "Failed to PMDeviceDiscovery : %d", res);
        goto error;
    }

    OCProvisionDev_t* pCurDev = pDeviceList;
    int i;
    while(pCurDev !=NULL)
    {
        for(i = 0; i < UUID_LENGTH; i++)
            printf("%c", pCurDev->doxm->deviceID.id[i]);
        printf("\n");
        pCurDev = pCurDev->next;
    }

    //Register callback function to each OxM
    OTMCallbackData_t justWorksCBData = {};
    justWorksCBData.loadSecretCB = LoadSecretJustWorksCallback;
    justWorksCBData.createSecureSessionCB = CreateSecureSessionJustWorksCallback;
    justWorksCBData.createSelectOxmPayloadCB = CreateJustWorksSelectOxmPayload;
    justWorksCBData.createOwnerTransferPayloadCB = CreateJustWorksOwnerTransferPayload;
    OTMSetOwnershipTransferCallbackData(OIC_JUST_WORKS, &justWorksCBData);

    // TODO: PIN based OxM implementation is required.
    //OTMCallbackData_t pinBasedCBData = {};
    //pinBasedCBData.loadSecretCB = InputPinCodeCallback;
    //pinBasedCBData.createSecureSessionCB = CreateSecureSessionRandomPinCallbak;
    //pinBasedCBData.createSelectOxmPayloadCB = CreatePinBasedSelectOxmPayload;
    //pinBasedCBData.createOwnerTransferPayloadCB = CreatePinBasedOwnerTransferPayload;
    //OTMSetOwnershipTransferCallbackData(OIC_RANDOM_DEVICE_PIN, &pinBasedCBData);

    char* myContext = "OTM Context";
    //Perform ownership transfer
    res = OCDoOwnershipTransfer((void*)myContext, pDeviceList, OwnershipTransferCB);
    if(OC_STACK_OK == res)
    {
        OC_LOG(INFO, TAG, "Request for ownership transfer is sent successfully.");
    }
    else
    {
        OC_LOG_V(ERROR, TAG, "Failed to OCDoOwnershipTransfer : %d", res);
    }

    gOwnershipState = 0;
    while ( gOwnershipState == 0 )
    {
        if (OCProcess() != OC_STACK_OK)
        {
            OC_LOG(ERROR, TAG, "OCStack process error");
            goto error;
        }
        sleep(1);
    }

// Credential & ACL provisioning between two devices.

    OCProvisionDev_t *pOwnedList = NULL;
    OCProvisionDev_t *pOwnedDevices [MAX_OWNED_DEVICE] = {0,};
    int nOwnedDevice = 0;

    res = OCDiscoverOwnedDevices(PREDEFINED_TIMEOUT, &pOwnedList);
    if (OC_STACK_OK == res)
    {
        printf("################## Owned Device List #######################\n");
        while (pOwnedList != NULL)
        {
            nOwnedDevice ++;
            printf(" %d : ", nOwnedDevice);
            for (int i = 0; i < UUID_LENGTH; i++)
            {
                printf("%c", pOwnedList->doxm->deviceID.id[i]);
            }
            printf("\n");
            pOwnedDevices[nOwnedDevice] = pOwnedList;
            pOwnedList = pOwnedList->next;
        }
    }
    else
    {
        OC_LOG(ERROR, TAG, "Error while Owned Device Discovery");
    }

    int Device1 = 0;
    int Device2 = 0;

    printf("Select 2 devices for Credential & ACL provisioning\n");
    printf("Device 1: ");
    unused = scanf("%d", &Device1);
    printf("Device 2: ");
    unused = scanf("%d", &Device2);


    gAcl1 = (OicSecAcl_t *)OICMalloc(sizeof(OicSecAcl_t));
    if (NULL == gAcl1)
    {
        OC_LOG(ERROR, TAG, "Error while memory allocation");
        goto error;
    }
    memset(gAcl1, 0x00, sizeof(OicSecAcl_t));

    gAcl2 = (OicSecAcl_t *)OICMalloc(sizeof(OicSecAcl_t));
    if (NULL == gAcl2)
    {
        OC_LOG(ERROR, TAG, "Error while memory allocation");
        goto error;
    }
    memset(gAcl2, 0x00, sizeof(OicSecAcl_t));

    printf("Input ACL for Device1\n");
    if ( 0 == InputACL(gAcl1))
    {
        printf("Success Input ACL\n");
    }
    else
    {
        OC_LOG(ERROR, TAG, "InputACL error");
        goto error;
    }

    printf("Input ACL for Device2\n");
    if (0 == InputACL(gAcl2))
    {
        printf("Success Input ACL\n");
    }
    else
    {
        OC_LOG(ERROR, TAG, "InputACL error");
        goto error;
    }
    char *ctx = "DUMMY";
    OCProvisionPairwiseDevices(ctx,SYMMETRIC_PAIR_WISE_KEY, OWNER_PSK_LENGTH_128, pOwnedDevices[Device1],
                           gAcl1, pOwnedDevices[Device2], gAcl2, ProvisionPairwiseCB);

    gOwnershipState = 0;
    while ( gOwnershipState == 0 )
    {
        if (OCProcess() != OC_STACK_OK)
        {
            OC_LOG(ERROR, TAG, "OCStack process error");
            goto error;
        }
        sleep(1);
    }

    if (OCStop() != OC_STACK_OK)
    {
        OC_LOG(ERROR, TAG, "OCStack process error");
        goto error;
    }

error:
    deleteACL(gAcl1);
    deleteACL(gAcl2);
    OCDeleteDiscoveredDevices(&pDeviceList);
    OCDeleteDiscoveredDevices(&pOwnedList);

    return 0;
}
