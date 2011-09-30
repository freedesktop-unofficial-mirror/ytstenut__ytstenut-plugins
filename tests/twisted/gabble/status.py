#!/usr/bin/env python
#
# Copyright (C) 2011 Intel Corp.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301 USA

import dbus

from gabbleservicetest import call_async, EventPattern, assertEquals, \
    ProxyWrapper, assertNotEquals
from gabbletest import exec_test, make_result_iq, acknowledge_iq
import gabbleconstants as cs
import yconstants as ycs
from gabblecaps_helper import *

from twisted.words.xish import xpath
from twisted.words.xish.domish import Element
from twisted.words.xish import domish

CAP_NAME = 'urn:ytstenut:capabilities:h264-over-ants'
CLIENT_NAME = 'fake-client'

client = 'http://telepathy.im/fake'
caps = {'ver': '0.1', 'node': client}
features = [
    ns.JINGLE_015,
    ns.JINGLE_015_AUDIO,
    ns.JINGLE_015_VIDEO,
    ns.GOOGLE_P2P,
    ycs.SERVICE_NS + '#the.target.service',
    CAP_NAME + '+notify'
    ]
identity = ['client/pc/en/Lolclient 0.L0L']

def check_pep_set(iq):
    pubsub = iq.children[0]
    publish = pubsub.children[0]
    item = publish.children[0]
    status_el = item.children[0]

    desc = None
    if status_el.children:
        desc = status_el.children[0]

    assertEquals('set', iq['type'])
    assertEquals(1, len(iq.children))

    assertEquals('pubsub', pubsub.name)
    assertEquals(ns.PUBSUB, pubsub.uri)
    assertEquals(1, len(pubsub.children))

    assertEquals('publish', publish.name)
    assertEquals(CAP_NAME, publish['node'])
    assertEquals(1, len(publish.children))

    assertEquals('item', item.name)
    assertEquals(1, len(item.children))

    assertEquals('status', status_el.name)

    if desc:
        assertEquals(1, len(status_el.children))

        assertEquals('description', desc.name)
        assertEquals(1, len(desc.children))

    return status_el, desc

def send_back_pep_event(stream, status_el):
    msg = Element((None, 'message'))
    msg['type'] = 'headline'
    msg['from'] = 'test@localhost'
    msg['to'] = 'test@localhost/Resource'
    msg['id'] = 'le-headline'
    event = msg.addElement('event')
    event['xmlns'] = ns.PUBSUB_EVENT
    items = event.addElement('items')
    items['node'] = CAP_NAME
    item = items.addElement('item')

    # just steal this
    item.addChild(status_el)

    # and go
    stream.send(msg)

def test(q, bus, conn, stream):
    # we won't be using any data forms, so these two shouldn't ever be
    # fired.
    q.forbid_events([EventPattern('dbus-signal', signal='ServiceAdded'),
                     EventPattern('dbus-signal', signal='ServiceRemoved')])

    call_async(q, conn.Future, 'EnsureSidecar', ycs.STATUS_IFACE)

    conn.Connect()

    # Now we're connected, the call we made earlier should return.
    e = q.expect('dbus-return', method='EnsureSidecar')
    path, props = e.value
    assertEquals({}, props)

    status = ProxyWrapper(bus.get_object(conn.bus_name, path),
                          ycs.STATUS_IFACE, {})

    # bad capability argument
    call_async(q, status, 'AdvertiseStatus', '', 'service.name', '')
    q.expect('dbus-error', method='AdvertiseStatus')

    # bad service name
    call_async(q, status, 'AdvertiseStatus', CAP_NAME, '', '')
    q.expect('dbus-error', method='AdvertiseStatus')

    # we can't test that the message type="headline" stanza is
    # actually received because it's thrown into the loopback stream
    # immediately.

    # announce a contact with the right caps
    bare_jid = "test-status@example.com"
    full_jid = bare_jid + "/BIGGESTRESOURCEEVAAAAHHH"

    contact_handle = conn.RequestHandles(cs.HT_CONTACT, [bare_jid])[0]

    presence_and_disco(q, conn, stream, full_jid, True, client, caps,
                       features, identity, {}, True, None)

    # this will be fired as text channel caps will be fired
    q.expect('dbus-signal', signal='ContactCapabilitiesChanged',
             predicate=lambda e: contact_handle in e.args[0])

    # okay now we know about the contact's caps, we can go ahead

    discovered = status.Get(ycs.STATUS_IFACE, 'DiscoveredStatuses',
                            dbus_interface=dbus.PROPERTIES_IFACE)
    assertEquals({}, discovered)

    el = Element(('urn:ytstenut:status', 'status'))
    el['activity'] = 'messing-with-your-stuff'
    desc = el.addElement('ytstenut:description', content='Yeah sorry about that')
    desc['xml:lang'] = 'en-GB'

    call_async(q, status, 'AdvertiseStatus', CAP_NAME,
               'ants.in.their.pants', el.toXml())

    e, _ = q.expect_many(EventPattern('stream-iq'),
                         EventPattern('dbus-return', method='AdvertiseStatus'))

    status_el, desc = check_pep_set(e.stanza)
    assertEquals('messing-with-your-stuff', status_el['activity'])
    assertEquals('ants.in.their.pants', status_el['from-service'])
    assertEquals(CAP_NAME, status_el['capability'])
    assertEquals('Yeah sorry about that', desc.children[0])

    acknowledge_iq(stream, e.stanza)
    send_back_pep_event(stream, status_el)

    sig = q.expect('dbus-signal', signal='StatusChanged',
                   interface=ycs.STATUS_IFACE)

    # check signal
    contact_id, capability, service_name, status_str = sig.args
    assertEquals(CAP_NAME, capability)
    assertEquals('ants.in.their.pants', service_name)
    assertNotEquals('', status_str)

    # check property
    discovered = status.Get(ycs.STATUS_IFACE, 'DiscoveredStatuses',
                            dbus_interface=dbus.PROPERTIES_IFACE)
    assertEquals({'test@localhost': {CAP_NAME: {'ants.in.their.pants': status_str}}},
                 discovered)

    # set another
    el = Element(('urn:ytstenut:status', 'status'))
    el['activity'] = 'rofling'
    desc = el.addElement('ytstenut:description', content='U MAD?')
    desc['xml:lang'] = 'en-GB'

    call_async(q, status, 'AdvertiseStatus', CAP_NAME,
               'bananaman.on.holiday', el.toXml())

    e, _ = q.expect_many(EventPattern('stream-iq'),
                         EventPattern('dbus-return', method='AdvertiseStatus'))

    status_el, desc = check_pep_set(e.stanza)
    assertEquals('rofling', status_el['activity'])
    assertEquals('bananaman.on.holiday', status_el['from-service'])
    assertEquals(CAP_NAME, status_el['capability'])
    assertEquals('U MAD?', desc.children[0])

    acknowledge_iq(stream, e.stanza)
    send_back_pep_event(stream, status_el)

    sig = q.expect('dbus-signal', signal='StatusChanged',
                   interface=ycs.STATUS_IFACE)

    # check signal
    contact_id, capability, service_name, bananaman_status_str = sig.args
    assertEquals(CAP_NAME, capability)
    assertEquals('bananaman.on.holiday', service_name)
    assertNotEquals('', bananaman_status_str)

    # check property
    discovered = status.Get(ycs.STATUS_IFACE, 'DiscoveredStatuses',
                            dbus_interface=dbus.PROPERTIES_IFACE)
    assertEquals({'test@localhost': {CAP_NAME: {
                    'ants.in.their.pants': status_str,
                    'bananaman.on.holiday': bananaman_status_str}}},
                 discovered)

    # unset the status from one service
    call_async(q, status, 'AdvertiseStatus', CAP_NAME,
               'ants.in.their.pants', '')

    e, _, = q.expect_many(EventPattern('stream-iq'),
                          EventPattern('dbus-return', method='AdvertiseStatus'))

    status_el, desc = check_pep_set(e.stanza)
    assertEquals('ants.in.their.pants', status_el['from-service'])
    assertEquals(CAP_NAME, status_el['capability'])
    assert 'activity' not in status_el.attributes
    assertEquals([], status_el.children)

    acknowledge_iq(stream, e.stanza)
    send_back_pep_event(stream, status_el)

    sig = q.expect('dbus-signal', signal='StatusChanged',
                   interface=ycs.STATUS_IFACE)

    # check signal
    contact_id, capability, service_name, status_str = sig.args
    assertEquals(CAP_NAME, capability)
    assertEquals('ants.in.their.pants', service_name)
    assertEquals('', status_str)

    # check property
    discovered = status.Get(ycs.STATUS_IFACE, 'DiscoveredStatuses',
                            dbus_interface=dbus.PROPERTIES_IFACE)
    assertEquals({'test@localhost': {CAP_NAME: {
                    'bananaman.on.holiday': bananaman_status_str}}},
                 discovered)

    # unset the status from the other service
    call_async(q, status, 'AdvertiseStatus', CAP_NAME,
               'bananaman.on.holiday', '')

    e, _ = q.expect_many(EventPattern('stream-iq'),
                         EventPattern('dbus-return', method='AdvertiseStatus'))

    # check message
    status_el, desc = check_pep_set(e.stanza)
    assertEquals('bananaman.on.holiday', status_el['from-service'])
    assertEquals(CAP_NAME, status_el['capability'])
    assert 'activity' not in status_el.attributes
    assertEquals([], status_el.children)

    acknowledge_iq(stream, e.stanza)
    send_back_pep_event(stream, status_el)

    sig = q.expect('dbus-signal', signal='StatusChanged',
                   interface=ycs.STATUS_IFACE)

    # check signal
    contact_id, capability, service_name, status_str = sig.args
    assertEquals(CAP_NAME, capability)
    assertEquals('bananaman.on.holiday', service_name)
    assertEquals('', status_str)

    # check property
    discovered = status.Get(ycs.STATUS_IFACE, 'DiscoveredStatuses',
                            dbus_interface=dbus.PROPERTIES_IFACE)
    assertEquals({}, discovered)

if __name__ == '__main__':
    exec_test(test, do_connect=False)
