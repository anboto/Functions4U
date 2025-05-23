// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 - 2022, the Anboto author and contributors
#include <Core/Core.h>

#include "StaticPlugin.h"

#include <Functions4U/EnableWarnings.h>

namespace Upp {

Array<StaticPlugin::PluginData>& StaticPlugin::Plugins() {
	static Array<PluginData> x;
	return x;
}

StaticPlugin::~StaticPlugin() {
	End();
}

void StaticPlugin::End() {
	if (!data)
		return;
	for (int i = 0; i < Plugins().GetCount(); ++i) {
		if (Plugins()[i].name == name && Plugins()[i].type == type && Plugins()[i].instance == instance) {
			Plugins()[i].Delete(data);
			data = 0;
			return;
		}
	}
	for (int i = 0; i < Plugins().GetCount(); ++i) {
		if (Plugins()[i].name == "" && Plugins()[i].type == type && Plugins()[i].instance == instance) {
			Plugins()[i].Delete(data);
			data = 0;
			return;
		}
	}	
}

bool StaticPlugin::_Init(const char *_name, const char *_type) {
	type = _type;
	End();
	for (int i = 0; i < Plugins().GetCount(); ++i) {
		if (Plugins()[i].name == _name && Plugins()[i].type == type) {
			data = Plugins()[i].New();
			name = _name;
			instance = this;
			Plugins()[i].instance = instance;
			return true;
		}
	}
	for (int i = 0; i < Plugins().GetCount(); ++i) {
		if (Plugins()[i].name == "" && Plugins()[i].type == type) {
			if (data)
				Plugins()[i].Delete(data);
			data = Plugins()[i].New();
			name = "";
			instance = this;
			Plugins()[i].instance = instance;
			return false;
		}
	}
	return false;
}

}
