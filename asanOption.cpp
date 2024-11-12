const char* __asan_default_options() {
	return "verbosity=1:malloc_context_size=20:strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_leaks=1:print_stats=1:atexit=1:detect_invalid_pointer_pairs=2:fast_unwind_on_malloc=0:alloc_dealloc_mismatch=1";
	}
