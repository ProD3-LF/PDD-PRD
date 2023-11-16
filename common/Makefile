#/* SPDX-License-Identifier: Apache-2.0 */
#/* Copyright (c) 2012-2023 Applied Communication Sciences
# * (now Peraton Labs Inc.)
# *
# * This software was developed in work supported by the following U.S.
# * Government contracts:
# *
# * HR0011-20-C-0160 HR0011-16-C-0061
# * 
# *
# * Any opinions, findings and conclusions or recommendations expressed in
# * this material are those of the author(s) and do not necessarily reflect
# * the views, either expressed or implied, of the U.S. Government.
# *
# * DoD Distribution Statement A
# * Approved for Public Release, Distribution Unlimited
# *
# * DISTAR Case 38846, cleared November 1, 2023
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# * http://www.apache.org/licenses/LICENSE-2.0
# */
# SPDX-License-Identifier: Apache-2.0 */
# Copyright (c) 2012-2023 Applied Communication Sciences
# (now Peraton Labs Inc.)
#
# This software was developed in work supported by the following U.S.
# Government contracts:
#
# HR0011-20-C-0160 HR0011-16-C-0061
# 
#
# Any opinions, findings and conclusions or recommendations expressed in
# this material are those of the author(s) and do not necessarily reflect
# the views, either expressed or implied, of the U.S. Government.
#
# DoD Distribution Statement A
# Approved for Public Release, Distribution Unlimited
#
# DISTAR Case 38846, cleared November 1, 2023
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
include $(BUILD)/Make.include.mk
include $(BUILD)/Make.recurse.mk
COMMONMODELDIR=../../../artifacts/central/models/common
.PHONY: clean 
all:

clean:
	rm -rf $(DIST_CONF)/central/models/common
install:
	rm -rf $(DIST_CONF)/central/models/common
	mkdir -p $(DIST_CONF)/central/models
	cp -r $(COMMONMODELDIR) $(DIST_CONF)/central/models
