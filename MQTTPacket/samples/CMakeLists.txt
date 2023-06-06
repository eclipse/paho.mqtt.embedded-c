#*******************************************************************************
#  Copyright (c) 2017 IBM Corp.
#
#  All rights reserved. This program and the accompanying materials
#  are made available under the terms of the Eclipse Public License v1.0
#  and Eclipse Distribution License v1.0 which accompany this distribution.
#
#  The Eclipse Public License is available at
#     http://www.eclipse.org/legal/epl-v10.html
#  and the Eclipse Distribution License is available at
#    http://www.eclipse.org/org/documents/edl-v10.php.
#
#  Contributors:
#     Ian Craggs - initial version
#*******************************************************************************/

# Samples

include_directories(../src)

add_executable(
  pub0sub1
  pub0sub1.c transport.c
)
target_link_libraries(pub0sub1 paho-embed-mqtt3c)

add_executable(
  pub0sub1_nb
  pub0sub1_nb.c transport.c
)
target_link_libraries(pub0sub1_nb paho-embed-mqtt3c)

add_executable(
  qos0pub
  qos0pub.c transport.c
)
target_link_libraries(qos0pub paho-embed-mqtt3c)

add_executable(
  ping
  ping.c transport.c
)
target_link_libraries(ping paho-embed-mqtt3c)

add_executable(
  ping_nb
  ping_nb.c transport.c
)
target_link_libraries(ping_nb paho-embed-mqtt3c)

add_executable(
v5pub0sub1
v5pub0sub1.c transport.c v5log.c
)
target_link_libraries(v5pub0sub1 paho-embed-mqtt5c)

add_executable(
v5pub0sub1_nb
v5pub0sub1_nb.c transport.c v5log.c
)
target_link_libraries(v5pub0sub1_nb paho-embed-mqtt5c)

add_executable(
v5qos0pub
v5qos0pub.c transport.c v5log.c
)
target_link_libraries(v5qos0pub paho-embed-mqtt5c)

add_executable(
  v5ping
  v5ping.c transport.c v5log.c
)
target_link_libraries(v5ping paho-embed-mqtt5c)

add_executable(
  v5ping_nb
  v5ping_nb.c transport.c v5log.c
)
target_link_libraries(v5ping_nb paho-embed-mqtt5c)
