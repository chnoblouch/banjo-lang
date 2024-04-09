#ifdef _WIN32
typedef unsigned long long u64;
#else
typedef unsigned long u64;
#endif

#if defined(_WIN32) && !defined(__MINGW32__)
#    define _CRT_SECURE_NO_WARNINGS
#    include <stdint.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>

#    include <windows.h>
#    include <dbghelp.h>

#    define MAX_NUM_FRAMES 256
#    define MAX_SYMBOL_LENGTH 255
#else
int printf(const char *format, ...);
int snprintf(char *buffer, u64 bufsz, const char *format, ...);
void *malloc(u64 size);
#endif

void ___banjo_print_string(const char *format, const char *string) {
    printf(format, string);
}

void ___banjo_print_address(const char *format, void *address) {
    printf(format, address);
}

char *___banjo_f64_to_string(double value) {
    int length = snprintf(0, 0, "%g", value);
    char *string = malloc(length + 1);
    snprintf(string, length + 1, "%g", value);
    return string;
}

typedef struct {
    u64 address;
    char *symbol;
} StackFrame;

#if defined(_WIN32) && !defined(__MINGW32__)

void ___acquire_stack_trace(StackFrame **frames, unsigned *num_frames) {
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    if (!SymInitialize(process, NULL, TRUE)) {
        printf("stack trace error: failed to initialized symbol handler\n");
        return;
    }

    CONTEXT context;
    RtlCaptureContext(&context);

    STACKFRAME64 frame;
    memset(&frame, 0, sizeof(frame));
    frame.AddrPC.Offset = context.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;

    IMAGEHLP_SYMBOL64 *symbol = malloc(sizeof(IMAGEHLP_SYMBOL64) + MAX_SYMBOL_LENGTH * sizeof(TCHAR));
    symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
    symbol->MaxNameLength = MAX_SYMBOL_LENGTH;

    *frames = malloc(MAX_NUM_FRAMES * sizeof(StackFrame));
    *num_frames = 0;

    while (1) {
        BOOL result = StackWalk64(
            IMAGE_FILE_MACHINE_AMD64,
            process,
            thread,
            &frame,
            &context,
            NULL,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            NULL
        );

        if (!result) {
            break;
        }

        DWORD64 displacement;
        if (SymGetSymFromAddr64(process, frame.AddrPC.Offset, &displacement, symbol)) {
            StackFrame *out_frame = *frames + (*num_frames)++;
            out_frame->address = frame.AddrPC.Offset;
            out_frame->symbol = malloc(MAX_SYMBOL_LENGTH + 1);
            strcpy(out_frame->symbol, symbol->Name);

            // printf("%llu\n", displacement);
        }
    }

    free(symbol);
}

#else
void ___acquire_stack_trace(StackFrame **frames, unsigned *num_frames) {
    *num_frames = 0;
}
#endif
