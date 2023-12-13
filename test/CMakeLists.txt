# SPDX-FileCopyrightText: 2016 Leslie Zhai <xiangzhai83@gmail.com>
# SPDX-License-Identifier: LGPL-2.0-or-later

ecm_add_test(
        devicestest.cpp
    LINK_LIBRARIES
        Qt6::Test
        KF6::Solid
)
