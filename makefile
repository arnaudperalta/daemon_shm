tar:
	$(MAKE) -C daemon_shm clean
	tar -zcf "$(CURDIR).tar.gz" demon/* client/* makefile README.md