#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI

import click
import ipaddress
import iptc
from pyroute2 import IPRoute
from pyroute2.netlink import NetlinkError


def handle_ip_string(ctx, param, value):
    try:
        ret = ipaddress.ip_network(value)
        return ret
    except ValueError:
        raise click.BadParameter(f'{value} is not a valid IP range.')


def iptables_add_masquerade(if_name, ip_range):
    chain = iptc.Chain(iptc.Table(iptc.Table.NAT), "POSTROUTING")
    rule = iptc.Rule()
    rule.src = ip_range
    rule.out_interface = if_name
    target = iptc.Target(rule, "MASQUERADE")
    rule.target = target
    chain.insert_rule(rule)


def iptables_allow_all(if_name):
    chain = iptc.Chain(iptc.Table(iptc.Table.FILTER), "INPUT")
    rule = iptc.Rule()
    rule.in_interface = if_name
    target = iptc.Target(rule, "ACCEPT")
    rule.target = target
    chain.insert_rule(rule)


@click.command()
@click.option("--if_name", default="ogstun", help="TUN interface name.")
@click.option("--ip_range", default='10.45.0.0/24', callback=handle_ip_string,
              help="IP range of the TUN interface.")
def main(if_name, ip_range):

    ipr = IPRoute()
    devs = ipr.link_lookup(ifname=if_name)
    if not devs:
        ipr.link('add', ifname=if_name, kind='tuntap', mode='tun')
        devs = ipr.link_lookup(ifname=if_name)
    dev = devs[0]
    ipr.link('set', index=dev, state='down')
    ipr.link('set', index=dev, state='up')

    for subnet in range(0, 256):
        # Get the first IP address in the IP range and netmask prefix length
        first_ip_addr = next(ip_range.hosts(), None) + (subnet * 256)
        if not first_ip_addr:
            raise ValueError('Invalid IP range.')
        else:
            first_ip_addr = first_ip_addr.exploded

        ip_netmask = ip_range.prefixlen

        # Add primary IP address, ignoring addresses already created by a prior run.
        try:
            ipr.addr('add', index=dev, address=first_ip_addr, mask=ip_netmask)
        except NetlinkError as e:
            if e.code != 17:
                raise

        subnet_route = ipaddress.ip_network(f'{first_ip_addr}/{ip_netmask}', strict=False).with_prefixlen
        try:
            ipr.route('add', dst=subnet_route, gateway=first_ip_addr)
        except NetlinkError as e:
            if e.code != 17:
                raise

        # setup iptables
        iptables_add_masquerade(if_name, ip_range.with_prefixlen)
        iptables_allow_all(if_name)
        # 'iptables -t nat -A POSTROUTING -s ' + ip_range.with_prefixlen + ' ! -o ' + if_name + ' -j MASQUERADE'

        # 'iptables -A INPUT -i ' + if_name + ' -j ACCEPT'


if __name__ == "__main__":
    main()