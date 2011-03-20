#!/usr/bin/env python
#
# Copyright (C) 2011 Collabora Ltd.
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
import dbus
import dbus.service

from servicetest import assertContains, assertEquals, assertSameSets
from servicetest import unwrap, ProxyWrapper, EventPattern, TimeoutError
from mctest import exec_test, create_fakecm_account
import constants as cs

YTST_ACCOUNT_MANAGER_PATH = "/com/meego/xpmn/ytstenut/AccountManager"
YTST_ACCOUNT_MANAGER_IFACE = "com.meego.xpmn.ytstenut.AccountManager"
YTST_ACCOUNT_PATH = "/org/freedesktop/Telepathy/Account/salut/local_ytstenut/automatic_account"

def is_account_going_online(event):
    presence = event.args[0].get("RequestedPresence")
    if presence and presence[0] == cs.PRESENCE_TYPE_AVAILABLE:
        return True
    return False

def is_account_going_offline(event):
    presence = event.args[0].get("RequestedPresence")
    if presence and presence[0] == cs.PRESENCE_TYPE_OFFLINE:
        return True
    return False

def test_account_exists(q, bus, mc):
    manager = bus.get_object(cs.MC, YTST_ACCOUNT_MANAGER_PATH)
    manager_props = dbus.Interface (manager, dbus.PROPERTIES_IFACE)

    account_path = manager_props.Get (YTST_ACCOUNT_MANAGER_IFACE, "Account")
    assertEquals (account_path, YTST_ACCOUNT_PATH)

    account = bus.get_object(cs.MC, account_path)
    account_props = dbus.Interface (account, dbus.PROPERTIES_IFACE)

    enabled = account_props.Get (cs.ACCOUNT, "Enabled")
    assertEquals(enabled, True)

def test_hold_and_release(q, bus, mc):
    q.timeout = 6    # Just longer than Release() timeout

    # Create an object that will proxy for a particular remote object.
    manager = bus.get_object(cs.MC, YTST_ACCOUNT_MANAGER_PATH)
    manager_ytst = dbus.Interface (manager, YTST_ACCOUNT_MANAGER_IFACE)

    manager_ytst.Hold()

    q.expect('dbus-signal', signal='AccountPropertyChanged',
             path=YTST_ACCOUNT_PATH, predicate=is_account_going_online)

    manager_ytst.Release()

    q.expect('dbus-signal', signal='AccountPropertyChanged',
             path=YTST_ACCOUNT_PATH, predicate=is_account_going_offline)

def test_hold_and_no_release(q, bus, mc):
    q.timeout = 6    # Just longer than Release() timeout

    # Create an object that will proxy for a particular remote object.
    manager = bus.get_object(cs.MC, YTST_ACCOUNT_MANAGER_PATH)
    manager_ytst = dbus.Interface (manager, YTST_ACCOUNT_MANAGER_IFACE)

    manager_ytst.Hold()

    q.expect('dbus-signal', signal='AccountPropertyChanged',
             path=YTST_ACCOUNT_PATH, predicate=is_account_going_online)

    # This second hold should cancel the release
    manager_ytst.Release()
    manager_ytst.Hold()

    try:
        q.expect('dbus-signal', signal='AccountPropertyChanged',
                 path=YTST_ACCOUNT_PATH, predicate=is_account_going_offline)
    except TimeoutError:
        pass
    else:
        assert False

if __name__ == '__main__':
    exec_test(test_account_exists, {})
    exec_test(test_hold_and_release, {})
    exec_test(test_hold_and_no_release, {})
