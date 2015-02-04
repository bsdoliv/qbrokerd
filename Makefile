.PHONY: distclean clean
all distclean clean:
	( cd brokerc && ${MAKE} $@ )
	( cd brokerd && ${MAKE} $@ )
