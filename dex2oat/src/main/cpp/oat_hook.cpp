#include <dlfcn.h>

#include <lsplt.hpp>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "logging.h"

// const std::string_view parameter_to_remove = " --inline-max-code-units=0";

#define DCL_HOOK_FUNC(ret, func, ...)                                                              \
    ret (*old_##func)(__VA_ARGS__);                                                                \
    ret new_##func(__VA_ARGS__)

DCL_HOOK_FUNC(size_t, _ZN3art15CompilerOptionsC1Ev, void *header) {
    size_t size = _ZN3art15CompilerOptionsC1Ev(header);

    for (int i = 0LL; i < 80 ; i += sizeof(void *)) {
        if (header + i == -1LL) {
            LOGI("Found a null pointer in CompilerOptions at offset %d", i);

            (header + i) = 0LL;

            return size;
        }
    }

    LOGE("CompilerOptions header is not null at any offset, this is unexpected");

    return size;
}

#undef DCL_HOOK_FUNC

void register_hook(dev_t dev, ino_t inode, const char* symbol, void* new_func, void** old_func) {
    LOGD("RegisterHook: %s, %p, %p", symbol, new_func, old_func);
    if (!lsplt::RegisterHook(dev, inode, symbol, new_func, old_func)) {
        LOGE("Failed to register plt_hook \"%s\"\n", symbol);
        return;
    }
}

#define PLT_HOOK_REGISTER_SYM(DEV, INODE, SYM, NAME)                                               \
    register_hook(DEV, INODE, SYM, reinterpret_cast<void*>(new_##NAME),                            \
                  reinterpret_cast<void**>(&old_##NAME))

#define PLT_HOOK_REGISTER(DEV, INODE, NAME) PLT_HOOK_REGISTER_SYM(DEV, INODE, #NAME, NAME)

__attribute__((constructor)) static void initialize() {
    dev_t dev = 0;
    ino_t inode = 0;
    for (auto& info : lsplt::MapInfo::Scan()) {
        if (info.path.starts_with("/apex/com.android.art/bin/dex2oat")) {
            dev = info.dev;
            inode = info.inode;
            break;
        }
    }

    PLT_HOOK_REGISTER(dev, inode, _ZN3art15CompilerOptionsC1Ev);
    if (lsplt::CommitHook()) {
        LOGD("lsplt hooks done");
    };
}
