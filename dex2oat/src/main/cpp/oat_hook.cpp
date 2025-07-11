#include <dlfcn.h>

#include <lsplt.hpp>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "logging.h"

#define DCL_HOOK_FUNC(ret, func, ...)                                                              \
    ret (*old_##func)(__VA_ARGS__);                                                                \
    ret new_##func(__VA_ARGS__)

// Small declaration of CompilerOptions to make the code less magic
struct CompilerOptions {
    const void *compiler_filter_;
    size_t huge_method_threshold_;
    size_t inline_max_code_units_;

    int instruction_set_;

    const void *instruction_set_features_;

    const void **no_inline_from_;
    size_t no_inline_from_count_;

    const void **dex_files_for_oat_file_;
    size_t dex_files_for_oat_file_count_;

    void *image_classes_;
    void *preloaded_classes_;

    uint8_t compiler_type_;

    uint8_t image_type_;

    bool multi_image_;
    bool compile_art_test_;
    bool emit_read_barrier_;
    bool baseline_;
    bool debuggable_;
    bool generate_debug_info_;
    bool generate_mini_debug_info_;
    bool generate_build_id_;
    bool implicit_null_checks_;
    bool implicit_so_checks_;
    bool implicit_suspend_checks_;
    bool compile_pic_;
    bool dump_timings_;
    bool dump_pass_timings_;
    bool dump_stats_;
    bool profile_branches_;

    const void *profile_compilation_info_;

    std::vector<std::string> verbose_methods_;
    size_t verbose_methods_count_;

    bool abort_on_hard_verifier_failure_;
    bool abort_on_soft_verifier_failure_;

    std::unique_ptr<std::ostream> init_failure_output_;

    std::string dump_cfg_file_name_;

    bool dump_cfg_append_;
    bool force_determinism_;
    bool check_linkage_conditions_;
    bool crash_on_linkage_violation_;
    bool deduplicate_code_;
    bool count_hotness_in_compiled_code_;
    bool resolve_startup_const_strings_;
    bool initialize_app_image_classes_;

    int check_profiled_methods_;

    uint32_t max_image_block_size_;

    std::vector<std::string> *passes_to_run_;
};

DCL_HOOK_FUNC(size_t, _ZN3art15CompilerOptionsC1Ev, void *header) {
    size_t size = old__ZN3art15CompilerOptionsC1Ev(header);

    size_t *inline_max_ptr = reinterpret_cast<size_t*>((uintptr_t)header + offsetof(CompilerOptions, inline_max_code_units_));
    LOGD("old inline_max_code_units: %zu", *inline_max_ptr);

    *inline_max_ptr = 0;

    LOGD("new inline_max_code_units: %zu", *inline_max_ptr);

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
