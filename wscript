#!/usr/bin/env python
import re

APPNAME = 'IndiePocket.lv2'
VERSION = '0.1.0'

top = '.'
out = 'build'

def options(opt):
    opt.load('compiler_c')

def require_pkg(cnf, pkg, version=None, alias=None):
    args = {
        'package': pkg,
        'args': '--cflags --libs',
        'mandatory': True
    }
    if version:
        args['atleast_version'] = version
    if alias:
        args['uselib_store'] = alias
    cnf.check_cfg(**args)
    cnf.check_cfg(modversion=pkg)

def configure(cnf):
    cnf.load('compiler_c')
    cnf.check(
        features='c cshlib',
        lib='m',
        cflags=['-Wall'],
        uselib_store='M'
    )
    require_pkg(cnf, 'lv2', '1.8.0', 'LV2')
    require_pkg(cnf, 'serd-0', '0.18.2', 'SERD')
    require_pkg(cnf, 'sord-0', '0.12.0', 'SORD')
    require_pkg(cnf, 'sndfile', '1.0.0', 'SNDFILE')
    require_pkg(cnf, 'gtk+-2.0', '2.18.0', 'GTK2')
    cnf.env.prepend_value(
        'CFLAGS',
        ['-Wall', '-Werror', '-Wextra', '-std=c99', '-fpic']
    )

def build(bld):
    lib_pattern = re.sub('^lib', '', bld.env.cshlib_PATTERN)

    bld.objects(
        source='pckt/kit.c pckt/drum.c pckt/sound.c pckt/sample.c',
        target='pckt_base',
        use='M'
    )
    bld.objects(
        source='pckt/sample_factory.c',
        target='pckt_sndfct',
        use='SNDFILE'
    )
    bld.objects(
        source='pckt/kit_factory.c',
        target='pckt_kitfct',
        use='SERD SORD',
        defines=['_BSD_SOURCE'] # for realpath
    )

    plugin = bld.shlib(
        source='lv2/indiepocket.c',
        target='%s/indiepocket' % APPNAME,
        use='pckt_base pckt_sndfct pckt_kitfct LV2'
    )
    plugin.env.cshlib_PATTERN = lib_pattern

    ui = bld.shlib(
        source='lv2/indiepocket_ui.c',
        target='%s/indiepocket_ui' % APPNAME,
        use='GTK2 LV2'
    )
    ui.env.cshlib_PATTERN = lib_pattern

    bld(
        rule='cp ${SRC} ${TGT}',
        source='lv2/manifest.ttl lv2/indiepocket.ttl',
        target='%s/' % APPNAME
    )
