/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2021
 *
 */

#include <phdk.h>
#include <settings.h>
#include "extnoti.h"

VOID NTAPI LoggedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

PPH_FILE_STREAM LogFileStream = NULL;
PH_CALLBACK_REGISTRATION LoggedCallbackRegistration;

VOID FileLogInitialization(
    VOID
    )
{
    NTSTATUS status;
    PPH_STRING fileName;
    PPH_STRING directory;

    fileName = PhaGetStringSetting(SETTING_NAME_LOG_FILENAME);

    if (!PhIsNullOrEmptyString(fileName))
    {
        fileName = PH_AUTO(PhExpandEnvironmentStrings(&fileName->sr));

        if (PhDetermineDosPathNameType(fileName->Buffer) == RtlPathTypeRelative)
        {
            directory = PH_AUTO(PhGetApplicationDirectory());
            fileName = PH_AUTO(PhConcatStringRef2(&directory->sr, &fileName->sr));
        }

        status = PhCreateFileStream(
            &LogFileStream,
            PhGetString(fileName),
            FILE_GENERIC_WRITE,
            FILE_SHARE_READ,
            FILE_OPEN_IF,
            PH_FILE_STREAM_APPEND | PH_FILE_STREAM_UNBUFFERED
            );

        if (NT_SUCCESS(status))
        {
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackLoggedEvent),
                LoggedCallback,
                NULL,
                &LoggedCallbackRegistration
                );
        }
    }
}

VOID NTAPI LoggedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_LOG_ENTRY logEntry = Parameter;

    if (logEntry)
    {
        PPH_STRING datetimeString;
        PPH_STRING messageString;
        SIZE_T returnLength;
        PH_FORMAT format[4];
        WCHAR formatBuffer[0x100];

        datetimeString = PhaFormatDateTime(NULL);
        messageString = PH_AUTO_T(PH_STRING, PhFormatLogEntry(logEntry));

        // %s: %s\r\n
        PhInitFormatSR(&format[0], datetimeString->sr);
        PhInitFormatS(&format[1], L": ");
        PhInitFormatSR(&format[2], messageString->sr);
        PhInitFormatS(&format[3], L"\r\n");

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &returnLength))
        {
            PH_STRINGREF messageSr;

            messageSr.Buffer = formatBuffer;
            messageSr.Length = returnLength - sizeof(UNICODE_NULL);

            PhWriteStringAsUtf8FileStream(LogFileStream, &messageSr);
        }
        else
        {
            PhWriteStringFormatAsUtf8FileStream(
                LogFileStream,
                L"%s: %s\r\n",
                PhGetString(datetimeString),
                PhGetString(messageString)
                );
        }
    }
}
