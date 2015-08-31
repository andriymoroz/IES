pkgconfiglibdir = $(libdir)/pkgconfig
pkgconfiglib_DATA = ies-api.pc

iesconfigdir = $(sysconfdir)/ies/
iesconfig_DATA = profile/environment

profileddir = $(sysconfdir)/profile.d
profiled_DATA = profile/ies.sh

acconfdir = $(sysconfdir)/ies/platforms/atwoodChannel
acconf_DATA = platforms/atwoodChannel/fm_platform_attributes.cfg

brconfdir = $(sysconfdir)/ies/platforms/boulderRapids
brconf_DATA = platforms/boulderRapids/fm_platform_attributes.cfg

rrconfdir = $(sysconfdir)/ies/platforms/rubyRapids
rrconf_DATA = platforms/rubyRapids/fm_platform_attributes.cfg

EXTRA_DIST = $(acconf_DATA) \
             $(brconf_DATA) \
             $(rrconf_DATA)

install-exec-hook:
	mkdir -p "$(DESTDIR)$(sysconfdir)/ies/platforms/boulderRapids"
	-( cd "$(DESTDIR)$(sysconfdir)/ies/platforms" && \
		ln -s boulderRapids default)
