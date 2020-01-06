APP_ROOT := $(CURDIR)
export APP_ROOT

#---------------------------- Configuration -----------------------------------#
## APP wide configuration
APP_CFLAGS = -ggdb3
FLTO_CFLAGS = -flto -ffat-lto-objects -fwhole-program

export FLTO_CFLAGS
export APP_CFLAGS

## DPDK configuration
RTE_SDK = $(APP_ROOT)/dpdk
RTE_TARGET = build

export RTE_SDK
export RTE_TARGET

## urcu configration
URCU_VERSION=v0.11.0

#------------------------------------------------------------------------------#

.NOTPARALLEL: prefix plat-setup
all:

clean: 

prefix:
	mkdir -p $(APP_ROOT)/external-lib/

clean_prefix:
	rm -rf $(APP_ROOT)/external-lib/

.PHONY: plat-setup
plat-setup:
	@git submodule deinit --all -f
ifeq ($(PLAT),t96)
	$(eval APP_CFLAGS += -march=armv8.2-a+crc+crypto+lse -mcpu=octeontx2)
	$(eval DPDK_TARGET = arm64-octeontx2-linuxapp-gcc)
else ifeq ($(PLAT), t83)
	$(eval APP_CFLAGS += -march=armv8-a+crc+crypto -mcpu=thunderx)
	$(eval DPDK_TARGET = arm64-thunderx-linuxapp-gcc)
else ifeq ($(PLAT), x86)
	$(eval DPDK_TARGET = x86_64-native-linux-gcc)
else
	$(error "Error: Unknown platform. Please use PLAT=t96/t83/x86 to specify the platform")
endif
	@git submodule sync
	@git submodule update --init --recursive --remote

urcu-setup: prefix
	cd $(APP_ROOT)/userspace-rcu && \
	git clean -dxf && \
	git reset --hard $(URCU_VERSION) && \
	cd $(APP_ROOT)/userspace-rcu && \
	$(APP_ROOT)/userspace-rcu/bootstrap && \
	$(APP_ROOT)/userspace-rcu/configure --prefix=$(APP_ROOT)/external-lib/  --enable-shared=no CFLAGS='-O3 -Ofast $(APP_CFLAGS) $(FLTO_CFLAGS)'

urcu:
	@$(MAKE) -C $(APP_ROOT)/userspace-rcu/
	@$(MAKE) install -C $(APP_ROOT)/userspace-rcu/

urcu-install: urcu-setup urcu

dpdk-setup: plat-setup prefix
	cd $(APP_ROOT)/dpdk 

.PHONY: dpdk
dpdk:
	cd $(APP_ROOT)/dpdk && \
	rm -rf build && \
	make config T=$(DPDK_TARGET) && \
	make -j EXTRA_CFLAGS="$(APP_CFLAGS)"

dpdk-rebuild:
	cd $(APP_ROOT)/dpdk && \
	make -j EXTRA_CFLAGS="$(APP_CFLAGS)"

dpdk-install: dpdk-setup dpdk

setup:	clean_prefix plat-setup urcu-install dpdk-install

tag:
	@rm -f tags
	@ctags $$(find . -name \*[.chS])
