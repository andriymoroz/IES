pkgconfiglibdir = $(libdir)/pkgconfig
pkgconfiglib_DATA = ies-api.pc

iesconfigdir = $(sysconfdir)/ies/
iesconfig_DATA = profile/environment

profileddir = $(sysconfdir)/profile.d
profiled_DATA = profile/ies.sh

acconfdir = $(sysconfdir)/ies/platforms/sdi_adapter_25g_ac
acconf_DATA = platforms/sdi_adapter_25g_ac/fm_platform_attributes.cfg

brconfdir = $(sysconfdir)/ies/platforms/sdi_adapter_100g_br
brconf_DATA = platforms/sdi_adapter_100g_br/fm_platform_attributes.cfg

rrconfdir = $(sysconfdir)/ies/platforms/sdv_100g_rr
rrconf_DATA = platforms/sdv_100g_rr/fm_platform_attributes.cfg

EXTRA_DIST = $(acconf_DATA) \
             $(brconf_DATA) \
             $(rrconf_DATA)

install-exec-hook:
	mkdir -p "$(DESTDIR)$(sysconfdir)/ies/platforms/sdi_adapter_100g_br"
	-( cd "$(DESTDIR)$(sysconfdir)/ies/platforms" && \
		ln -s sdi_adapter_100g_br default)
