ARCH=x86
subdir=./src
subdir2=./testing

all:
	@make -C  $(subdir)  ARCH=$(ARCH) CCPRE=$(CCPRE)
	@make -C  $(subdir2)  ARCH=$(ARCH) CCPRE=$(CCPRE)
clean:
	@make clean -C  $(subdir)  ARCH=$(ARCH) CCPRE=$(CCPRE)
	@make clean -C  $(subdir2)  ARCH=$(ARCH) CCPRE=$(CCPRE)


