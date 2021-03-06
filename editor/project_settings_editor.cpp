/*************************************************************************/
/*  project_settings.cpp                                                 */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                    http://www.godotengine.org                         */
/*************************************************************************/
/* Copyright (c) 2007-2017 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2017 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/
#include "project_settings_editor.h"

#include "editor_node.h"
#include "global_constants.h"
#include "os/keyboard.h"
#include "project_settings.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/tab_container.h"
#include "translation.h"

ProjectSettingsEditor *ProjectSettingsEditor::singleton = NULL;

static const char *_button_names[JOY_BUTTON_MAX] = {
	"PS Cross, XBox A, Nintendo B",
	"PS Circle, XBox B, Nintendo A",
	"PS Square, XBox X, Nintendo Y",
	"PS Triangle, XBox Y, Nintendo X",
	"L, L1",
	"R, R1",
	"L2",
	"R2",
	"L3",
	"R3",
	"Select, Nintendo -",
	"Start, Nintendo +",
	"D-Pad Up",
	"D-Pad Down",
	"D-Pad Left",
	"D-Pad Right"
};

static const char *_axis_names[JOY_AXIS_MAX * 2] = {
	" (Left Stick Left)",
	" (Left Stick Right)",
	" (Left Stick Up)",
	" (Left Stick Down)",
	" (Right Stick Left)",
	" (Right Stick Right)",
	" (Right Stick Up)",
	" (Right Stick Down)",
	"", "", "", "",
	"", " (L2)",
	"", " (R2)"
};

void ProjectSettingsEditor::_notification(int p_what) {

	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			globals_editor->edit(ProjectSettings::get_singleton());

			search_button->set_icon(get_icon("Search", "EditorIcons"));
			clear_button->set_icon(get_icon("Close", "EditorIcons"));

			translation_list->connect("button_pressed", this, "_translation_delete");
			_update_actions();
			popup_add->add_icon_item(get_icon("Keyboard", "EditorIcons"), TTR("Key "), INPUT_KEY); //"Key " - because the word 'key' has already been used as a key animation
			popup_add->add_icon_item(get_icon("JoyButton", "EditorIcons"), TTR("Joy Button"), INPUT_JOY_BUTTON);
			popup_add->add_icon_item(get_icon("JoyAxis", "EditorIcons"), TTR("Joy Axis"), INPUT_JOY_MOTION);
			popup_add->add_icon_item(get_icon("Mouse", "EditorIcons"), TTR("Mouse Button"), INPUT_MOUSE_BUTTON);

			List<String> tfn;
			ResourceLoader::get_recognized_extensions_for_type("Translation", &tfn);
			for (List<String>::Element *E = tfn.front(); E; E = E->next()) {

				translation_file_open->add_filter("*." + E->get());
			}

			List<String> rfn;
			ResourceLoader::get_recognized_extensions_for_type("Resource", &rfn);
			for (List<String>::Element *E = rfn.front(); E; E = E->next()) {

				translation_res_file_open->add_filter("*." + E->get());
				translation_res_option_file_open->add_filter("*." + E->get());
			}
		} break;
		case NOTIFICATION_POPUP_HIDE: {
			EditorSettings::get_singleton()->set("interface/dialogs/project_settings_bounds", get_rect());
		} break;
		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			_update_actions();
		} break;
	}
}

void ProjectSettingsEditor::_action_selected() {

	TreeItem *ti = input_editor->get_selected();
	if (!ti || !ti->is_editable(0))
		return;

	add_at = "input/" + ti->get_text(0);
	edit_idx = -1;
}

void ProjectSettingsEditor::_action_edited() {

	TreeItem *ti = input_editor->get_selected();
	if (!ti)
		return;

	String new_name = ti->get_text(0);
	String old_name = add_at.substr(add_at.find("/") + 1, add_at.length());

	if (new_name == old_name)
		return;

	if (new_name.find("/") != -1 || new_name.find(":") != -1 || new_name == "") {

		ti->set_text(0, old_name);
		add_at = "input/" + old_name;

		message->set_text(TTR("Invalid action (anything goes but '/' or ':')."));
		message->popup_centered(Size2(300, 100) * EDSCALE);
		return;
	}

	String action_prop = "input/" + new_name;

	if (ProjectSettings::get_singleton()->has(action_prop)) {

		ti->set_text(0, old_name);
		add_at = "input/" + old_name;

		message->set_text(vformat(TTR("Action '%s' already exists!"), new_name));
		message->popup_centered(Size2(300, 100) * EDSCALE);
		return;
	}

	int order = ProjectSettings::get_singleton()->get_order(add_at);
	Array va = ProjectSettings::get_singleton()->get(add_at);

	setting = true;
	undo_redo->create_action(TTR("Rename Input Action Event"));
	undo_redo->add_do_method(ProjectSettings::get_singleton(), "clear", add_at);
	undo_redo->add_do_method(ProjectSettings::get_singleton(), "set", action_prop, va);
	undo_redo->add_do_method(ProjectSettings::get_singleton(), "set_order", action_prop, order);
	undo_redo->add_undo_method(ProjectSettings::get_singleton(), "clear", action_prop);
	undo_redo->add_undo_method(ProjectSettings::get_singleton(), "set", add_at, va);
	undo_redo->add_undo_method(ProjectSettings::get_singleton(), "set_order", add_at, order);
	undo_redo->add_do_method(this, "_update_actions");
	undo_redo->add_undo_method(this, "_update_actions");
	undo_redo->add_do_method(this, "_settings_changed");
	undo_redo->add_undo_method(this, "_settings_changed");
	undo_redo->commit_action();
	setting = false;

	add_at = action_prop;
}

void ProjectSettingsEditor::_device_input_add() {

	Ref<InputEvent> ie;
	String name = add_at;
	int idx = edit_idx;
	Variant old_val = ProjectSettings::get_singleton()->get(name);
	Array arr = old_val;
	//	ie.device = device_id->get_value();
	//	ie.type = add_type;

	switch (add_type) {

		case INPUT_MOUSE_BUTTON: {

			Ref<InputEventMouseButton> mb;
			mb.instance();
			mb->set_button_index(device_index->get_selected() + 1);
			mb->set_device(device_id->get_value());

			for (int i = 0; i < arr.size(); i++) {

				Ref<InputEventMouseButton> aie = arr[i];
				if (aie.is_null())
					continue;
				if (aie->get_device() == mb->get_device() && aie->get_button_index() == mb->get_button_index()) {
					return;
				}
			}

			ie = mb;

		} break;
		case INPUT_JOY_MOTION: {

			Ref<InputEventJoypadMotion> jm;
			jm.instance();
			jm->set_axis(device_index->get_selected() >> 1);
			jm->set_axis_value(device_index->get_selected() & 1 ? 1 : -1);
			jm->set_device(device_id->get_value());

			for (int i = 0; i < arr.size(); i++) {

				Ref<InputEventJoypadMotion> aie = arr[i];
				if (aie.is_null())
					continue;
				if (aie->get_device() == jm->get_device() && aie->get_axis() == jm->get_axis() && aie->get_axis_value() == jm->get_axis_value()) {
					return;
				}
			}

			ie = jm;

		} break;
		case INPUT_JOY_BUTTON: {

			Ref<InputEventJoypadButton> jb;
			jb.instance();

			jb->set_button_index(device_index->get_selected());
			jb->set_device(device_id->get_value());

			for (int i = 0; i < arr.size(); i++) {

				Ref<InputEventJoypadButton> aie = arr[i];
				if (aie.is_null())
					continue;
				if (aie->get_device() == jb->get_device() && aie->get_button_index() == jb->get_button_index()) {
					return;
				}
			}
			ie = jb;

		} break;
		default: {}
	}

	if (idx < 0 || idx >= arr.size()) {
		arr.push_back(ie);
	} else {
		arr[idx] = ie;
	}

	undo_redo->create_action(TTR("Add Input Action Event"));
	undo_redo->add_do_method(ProjectSettings::get_singleton(), "set", name, arr);
	undo_redo->add_undo_method(ProjectSettings::get_singleton(), "set", name, old_val);
	undo_redo->add_do_method(this, "_update_actions");
	undo_redo->add_undo_method(this, "_update_actions");
	undo_redo->add_do_method(this, "_settings_changed");
	undo_redo->add_undo_method(this, "_settings_changed");
	undo_redo->commit_action();

	_show_last_added(ie, name);
}

void ProjectSettingsEditor::_press_a_key_confirm() {

	if (last_wait_for_key.is_null())
		return;

	Ref<InputEventKey> ie;
	ie.instance();
	ie->set_scancode(last_wait_for_key->get_scancode());
	ie->set_shift(last_wait_for_key->get_shift());
	ie->set_alt(last_wait_for_key->get_alt());
	ie->set_control(last_wait_for_key->get_control());
	ie->set_metakey(last_wait_for_key->get_metakey());

	String name = add_at;
	int idx = edit_idx;

	Variant old_val = ProjectSettings::get_singleton()->get(name);
	Array arr = old_val;

	for (int i = 0; i < arr.size(); i++) {

		Ref<InputEventKey> aie = arr[i];
		if (aie.is_null())
			continue;
		if (aie->get_scancode_with_modifiers() == ie->get_scancode_with_modifiers()) {
			return;
		}
	}

	if (idx < 0 || idx >= arr.size()) {
		arr.push_back(ie);
	} else {
		arr[idx] = ie;
	}

	undo_redo->create_action(TTR("Add Input Action Event"));
	undo_redo->add_do_method(ProjectSettings::get_singleton(), "set", name, arr);
	undo_redo->add_undo_method(ProjectSettings::get_singleton(), "set", name, old_val);
	undo_redo->add_do_method(this, "_update_actions");
	undo_redo->add_undo_method(this, "_update_actions");
	undo_redo->add_do_method(this, "_settings_changed");
	undo_redo->add_undo_method(this, "_settings_changed");
	undo_redo->commit_action();

	_show_last_added(ie, name);
}

void ProjectSettingsEditor::_show_last_added(const Ref<InputEvent> &p_event, const String &p_name) {
	TreeItem *r = input_editor->get_root();

	String name = p_name;
	name.erase(0, 6);
	if (!r)
		return;
	r = r->get_children();
	if (!r)
		return;
	bool found = false;
	while (r) {
		if (r->get_text(0) != name) {
			r = r->get_next();
			continue;
		}
		TreeItem *child = r->get_children();
		while (child) {
			Variant input = child->get_meta("__input");
			if (p_event == input) {
				child->select(0);
				found = true;
				break;
			}
			child = child->get_next();
		}
		if (found) break;
		r = r->get_next();
	}

	if (found) input_editor->ensure_cursor_is_visible();
}

void ProjectSettingsEditor::_wait_for_key(const Ref<InputEvent> &p_event) {

	Ref<InputEventKey> k = p_event;

	if (k.is_valid() && k->is_pressed() && k->get_scancode() != 0) {

		last_wait_for_key = p_event;
		String str = keycode_get_string(k->get_scancode()).capitalize();
		if (k->get_metakey())
			str = TTR("Meta+") + str;
		if (k->get_shift())
			str = TTR("Shift+") + str;
		if (k->get_alt())
			str = TTR("Alt+") + str;
		if (k->get_control())
			str = TTR("Control+") + str;

		press_a_key_label->set_text(str);
		press_a_key->accept_event();
	}
}

void ProjectSettingsEditor::_add_item(int p_item, Ref<InputEvent> p_exiting_event) {

	add_type = InputType(p_item);

	switch (add_type) {

		case INPUT_KEY: {

			press_a_key_label->set_text(TTR("Press a Key.."));
			last_wait_for_key = Ref<InputEvent>();
			press_a_key->popup_centered(Size2(250, 80) * EDSCALE);
			press_a_key->grab_focus();
		} break;
		case INPUT_MOUSE_BUTTON: {

			device_index_label->set_text(TTR("Mouse Button Index:"));
			device_index->clear();
			device_index->add_item(TTR("Left Button"));
			device_index->add_item(TTR("Right Button"));
			device_index->add_item(TTR("Middle Button"));
			device_index->add_item(TTR("Wheel Up Button"));
			device_index->add_item(TTR("Wheel Down Button"));
			device_index->add_item(TTR("Button 6"));
			device_index->add_item(TTR("Button 7"));
			device_index->add_item(TTR("Button 8"));
			device_index->add_item(TTR("Button 9"));
			device_input->popup_centered_minsize(Size2(350, 95) * EDSCALE);

			Ref<InputEventMouseButton> mb = p_exiting_event;
			if (mb.is_valid()) {
				device_index->select(mb->get_button_index() - 1);
				device_id->set_value(mb->get_device());
				device_input->get_ok()->set_text(TTR("Change"));
			} else {
				device_id->set_value(0);
				device_input->get_ok()->set_text(TTR("Add"));
			}
		} break;
		case INPUT_JOY_MOTION: {

			device_index_label->set_text(TTR("Joypad Axis Index:"));
			device_index->clear();
			for (int i = 0; i < JOY_AXIS_MAX * 2; i++) {

				String desc = _axis_names[i];
				device_index->add_item(TTR("Axis") + " " + itos(i / 2) + " " + (i & 1 ? "+" : "-") + desc);
			}
			device_input->popup_centered_minsize(Size2(350, 95) * EDSCALE);

			Ref<InputEventJoypadMotion> jm = p_exiting_event;
			if (jm.is_valid()) {
				device_index->select(jm->get_axis() * 2 + (jm->get_axis_value() > 0 ? 1 : 0));
				device_id->set_value(jm->get_device());
				device_input->get_ok()->set_text(TTR("Change"));
			} else {
				device_id->set_value(0);
				device_input->get_ok()->set_text(TTR("Add"));
			}
		} break;
		case INPUT_JOY_BUTTON: {

			device_index_label->set_text(TTR("Joypad Button Index:"));
			device_index->clear();

			for (int i = 0; i < JOY_BUTTON_MAX; i++) {

				device_index->add_item(itos(i) + ": " + String(_button_names[i]));
			}
			device_input->popup_centered_minsize(Size2(350, 95) * EDSCALE);

			Ref<InputEventJoypadButton> jb = p_exiting_event;
			if (jb.is_valid()) {
				device_index->select(jb->get_button_index());
				device_id->set_value(jb->get_device());
				device_input->get_ok()->set_text(TTR("Change"));
			} else {
				device_id->set_value(0);
				device_input->get_ok()->set_text(TTR("Add"));
			}

		} break;
		default: {}
	}
}

void ProjectSettingsEditor::_edit_item(Ref<InputEvent> p_exiting_event) {

	InputType ie_type;

	if ((Ref<InputEventKey>(p_exiting_event)).is_valid()) {
		ie_type = INPUT_KEY;

	} else if ((Ref<InputEventJoypadButton>(p_exiting_event)).is_valid()) {
		ie_type = INPUT_JOY_BUTTON;

	} else if ((Ref<InputEventMouseButton>(p_exiting_event)).is_valid()) {
		ie_type = INPUT_MOUSE_BUTTON;

	} else if ((Ref<InputEventJoypadMotion>(p_exiting_event)).is_valid()) {
		ie_type = INPUT_JOY_MOTION;

	} else {
		return;
	}

	_add_item(ie_type, p_exiting_event);
}
void ProjectSettingsEditor::_action_activated() {

	TreeItem *ti = input_editor->get_selected();

	if (!ti || ti->get_parent() == input_editor->get_root())
		return;

	String name = "input/" + ti->get_parent()->get_text(0);
	int idx = ti->get_metadata(0);
	Array va = ProjectSettings::get_singleton()->get(name);

	ERR_FAIL_INDEX(idx, va.size());

	Ref<InputEvent> ie = va[idx];

	if (ie.is_null())
		return;

	add_at = name;
	edit_idx = idx;
	_edit_item(ie);
}

void ProjectSettingsEditor::_action_button_pressed(Object *p_obj, int p_column, int p_id) {

	TreeItem *ti = p_obj->cast_to<TreeItem>();

	ERR_FAIL_COND(!ti);

	if (p_id == 1) {
		Point2 ofs = input_editor->get_global_position();
		Rect2 ir = input_editor->get_item_rect(ti);
		ir.position.y -= input_editor->get_scroll().y;
		ofs += ir.position + ir.size;
		ofs.x -= 100;
		popup_add->set_position(ofs);
		popup_add->popup();
		add_at = "input/" + ti->get_text(0);
		edit_idx = -1;

	} else if (p_id == 2) {
		//remove

		if (ti->get_parent() == input_editor->get_root()) {

			//remove main thing

			String name = "input/" + ti->get_text(0);
			Variant old_val = ProjectSettings::get_singleton()->get(name);
			int order = ProjectSettings::get_singleton()->get_order(name);

			undo_redo->create_action(TTR("Add Input Action"));
			undo_redo->add_do_method(ProjectSettings::get_singleton(), "clear", name);
			undo_redo->add_undo_method(ProjectSettings::get_singleton(), "set", name, old_val);
			undo_redo->add_undo_method(ProjectSettings::get_singleton(), "set_order", name, order);
			undo_redo->add_do_method(this, "_update_actions");
			undo_redo->add_undo_method(this, "_update_actions");
			undo_redo->add_do_method(this, "_settings_changed");
			undo_redo->add_undo_method(this, "_settings_changed");
			undo_redo->commit_action();

		} else {
			//remove action
			String name = "input/" + ti->get_parent()->get_text(0);
			Variant old_val = ProjectSettings::get_singleton()->get(name);
			int idx = ti->get_metadata(0);

			Array va = old_val;

			ERR_FAIL_INDEX(idx, va.size());

			for (int i = idx; i < va.size() - 1; i++) {

				va[i] = va[i + 1];
			}

			va.resize(va.size() - 1);

			undo_redo->create_action(TTR("Erase Input Action Event"));
			undo_redo->add_do_method(ProjectSettings::get_singleton(), "set", name, va);
			undo_redo->add_undo_method(ProjectSettings::get_singleton(), "set", name, old_val);
			undo_redo->add_do_method(this, "_update_actions");
			undo_redo->add_undo_method(this, "_update_actions");
			undo_redo->add_do_method(this, "_settings_changed");
			undo_redo->add_undo_method(this, "_settings_changed");
			undo_redo->commit_action();
		}
	} else if (p_id == 3) {
		//edit

		if (ti->get_parent() == input_editor->get_root()) {

			ti->set_as_cursor(0);
			input_editor->edit_selected();

		} else {
			//edit action
			String name = "input/" + ti->get_parent()->get_text(0);
			int idx = ti->get_metadata(0);
			Array va = ProjectSettings::get_singleton()->get(name);

			ERR_FAIL_INDEX(idx, va.size());

			Ref<InputEvent> ie = va[idx];

			if (ie.is_null())
				return;

			ti->set_as_cursor(0);
			add_at = name;
			edit_idx = idx;
			_edit_item(ie);
		}
	}
}

void ProjectSettingsEditor::_update_actions() {

	if (setting)
		return;

	input_editor->clear();
	TreeItem *root = input_editor->create_item();
	input_editor->set_hide_root(true);

	List<PropertyInfo> props;
	ProjectSettings::get_singleton()->get_property_list(&props);

	for (List<PropertyInfo>::Element *E = props.front(); E; E = E->next()) {

		const PropertyInfo &pi = E->get();
		if (!pi.name.begins_with("input/"))
			continue;

		String name = pi.name.get_slice("/", 1);
		if (name == "")
			continue;

		TreeItem *item = input_editor->create_item(root);
		//item->set_cell_mode(0,TreeItem::CELL_MODE_CHECK);
		item->set_text(0, name);
		item->add_button(0, get_icon("Add", "EditorIcons"), 1, false, TTR("Add Event"));
		if (!ProjectSettings::get_singleton()->get_input_presets().find(pi.name)) {
			item->add_button(0, get_icon("Remove", "EditorIcons"), 2, false, TTR("Remove"));
			item->set_editable(0, true);
		}
		item->set_custom_bg_color(0, get_color("prop_subsection", "Editor"));
		//item->set_checked(0,pi.usage&PROPERTY_USAGE_CHECKED);

		Array actions = ProjectSettings::get_singleton()->get(pi.name);

		for (int i = 0; i < actions.size(); i++) {

			Ref<InputEvent> ie = actions[i];
			if (ie.is_null())
				continue;

			TreeItem *action = input_editor->create_item(item);

			Ref<InputEventKey> k = ie;
			if (k.is_valid()) {

				String str = keycode_get_string(k->get_scancode()).capitalize();
				if (k->get_metakey())
					str = TTR("Meta+") + str;
				if (k->get_shift())
					str = TTR("Shift+") + str;
				if (k->get_alt())
					str = TTR("Alt+") + str;
				if (k->get_control())
					str = TTR("Control+") + str;

				action->set_text(0, str);
				action->set_icon(0, get_icon("Keyboard", "EditorIcons"));
			}

			Ref<InputEventJoypadButton> jb = ie;

			if (jb.is_valid()) {

				String str = TTR("Device") + " " + itos(jb->get_device()) + ", " + TTR("Button") + " " + itos(jb->get_button_index());
				if (jb->get_button_index() >= 0 && jb->get_button_index() < JOY_BUTTON_MAX)
					str += String() + " (" + _button_names[jb->get_button_index()] + ").";
				else
					str += ".";

				action->set_text(0, str);
				action->set_icon(0, get_icon("JoyButton", "EditorIcons"));
			}

			Ref<InputEventMouseButton> mb = ie;

			if (mb.is_valid()) {
				String str = TTR("Device") + " " + itos(mb->get_device()) + ", ";
				switch (mb->get_button_index()) {
					case BUTTON_LEFT: str += TTR("Left Button."); break;
					case BUTTON_RIGHT: str += TTR("Right Button."); break;
					case BUTTON_MIDDLE: str += TTR("Middle Button."); break;
					case BUTTON_WHEEL_UP: str += TTR("Wheel Up."); break;
					case BUTTON_WHEEL_DOWN: str += TTR("Wheel Down."); break;
					default: str += TTR("Button") + " " + itos(mb->get_button_index()) + ".";
				}

				action->set_text(0, str);
				action->set_icon(0, get_icon("Mouse", "EditorIcons"));
			}

			Ref<InputEventJoypadMotion> jm = ie;

			if (jm.is_valid()) {

				int ax = jm->get_axis();
				int n = 2 * ax + (jm->get_axis_value() < 0 ? 0 : 1);
				String desc = _axis_names[n];
				String str = TTR("Device") + " " + itos(jm->get_device()) + ", " + TTR("Axis") + " " + itos(ax) + " " + (jm->get_axis_value() < 0 ? "-" : "+") + desc + ".";
				action->set_text(0, str);
				action->set_icon(0, get_icon("JoyAxis", "EditorIcons"));
			}
			action->add_button(0, get_icon("Edit", "EditorIcons"), 3, false, TTR("Edit"));
			action->add_button(0, get_icon("Remove", "EditorIcons"), 2, false, TTR("Remove"));
			action->set_metadata(0, i);
			action->set_meta("__input", ie);
		}
	}
}

void ProjectSettingsEditor::popup_project_settings() {

	// Restore valid window bounds or pop up at default size.
	if (EditorSettings::get_singleton()->has("interface/dialogs/project_settings_bounds")) {
		popup(EditorSettings::get_singleton()->get("interface/dialogs/project_settings_bounds"));
	} else {
		popup_centered_ratio();
	}
	globals_editor->update_category_list();
	_update_translations();
	autoload_settings->update_autoload();
	plugin_settings->update_plugins();
}

void ProjectSettingsEditor::_item_selected() {

	TreeItem *ti = globals_editor->get_property_editor()->get_scene_tree()->get_selected();
	if (!ti)
		return;
	if (!ti->get_parent())
		return;
	category->set_text(globals_editor->get_current_section());
	property->set_text(ti->get_text(0));
	popup_copy_to_feature->set_disabled(false);
}

void ProjectSettingsEditor::_item_adds(String) {

	_item_add();
}

void ProjectSettingsEditor::_item_add() {

	Variant value;
	switch (type->get_selected()) {
		case 0: value = false; break;
		case 1: value = 0; break;
		case 2: value = 0.0; break;
		case 3: value = ""; break;
	}

	String catname = category->get_text().strip_edges();
	/*if (!catname.is_valid_identifier()) {
		message->set_text("Invalid Category.\nValid characters: a-z,A-Z,0-9 or _");
		message->popup_centered(Size2(300,100));
		return;
	}*/

	String propname = property->get_text().strip_edges();
	/*if (!propname.is_valid_identifier()) {
		message->set_text("Invalid Property.\nValid characters: a-z,A-Z,0-9 or _");
		message->popup_centered(Size2(300,100));
		return;
	}*/

	String name = catname != "" ? catname + "/" + propname : propname;

	undo_redo->create_action("Add Global Property");

	undo_redo->add_do_property(ProjectSettings::get_singleton(), name, value);

	if (ProjectSettings::get_singleton()->has(name)) {
		undo_redo->add_undo_property(ProjectSettings::get_singleton(), name, ProjectSettings::get_singleton()->get(name));
	} else {
		undo_redo->add_undo_property(ProjectSettings::get_singleton(), name, Variant());
	}

	undo_redo->add_do_method(globals_editor, "update_category_list");
	undo_redo->add_undo_method(globals_editor, "update_category_list");

	undo_redo->add_do_method(this, "_settings_changed");
	undo_redo->add_undo_method(this, "_settings_changed");

	undo_redo->commit_action();

	globals_editor->set_current_section(catname);

	_settings_changed();
}

void ProjectSettingsEditor::_item_del() {

	String path = globals_editor->get_property_editor()->get_selected_path();
	if (path == String()) {
		EditorNode::get_singleton()->show_warning(TTR("Select an setting item first!"));
		return;
	}

	String property = globals_editor->get_current_section().plus_file(path);

	if (!ProjectSettings::get_singleton()->has(property)) {
		EditorNode::get_singleton()->show_warning(TTR("No property '" + property + "' exists."));
		return;
	}

	if (ProjectSettings::get_singleton()->get_order(property) < ProjectSettings::NO_BUILTIN_ORDER_BASE) {
		EditorNode::get_singleton()->show_warning(TTR("Setting '" + property + "' is internal, and it can't be deleted."));
		return;
	}

	print_line("to delete.. " + property);
	undo_redo->create_action(TTR("Delete Item"));

	Variant value = ProjectSettings::get_singleton()->get(property);
	int order = ProjectSettings::get_singleton()->get_order(property);

	undo_redo->add_do_method(ProjectSettings::get_singleton(), "clear", property);
	undo_redo->add_undo_method(ProjectSettings::get_singleton(), "set", property, value);
	undo_redo->add_undo_method(ProjectSettings::get_singleton(), "set_order", property, order);

	undo_redo->add_do_method(globals_editor, "update_category_list");
	undo_redo->add_undo_method(globals_editor, "update_category_list");

	undo_redo->add_do_method(this, "_settings_changed");
	undo_redo->add_undo_method(this, "_settings_changed");

	undo_redo->commit_action();
}

void ProjectSettingsEditor::_action_adds(String) {

	_action_add();
}

void ProjectSettingsEditor::_action_add() {

	String action = action_name->get_text();
	if (action.find("/") != -1 || action.find(":") != -1 || action == "") {
		message->set_text(TTR("Invalid action (anything goes but '/' or ':')."));
		message->popup_centered(Size2(300, 100) * EDSCALE);
		return;
	}

	if (ProjectSettings::get_singleton()->has("input/" + action)) {
		message->set_text(vformat(TTR("Action '%s' already exists!"), action));
		message->popup_centered(Size2(300, 100) * EDSCALE);
		return;
	}

	Array va;
	String name = "input/" + action;
	undo_redo->create_action(TTR("Add Input Action Event"));
	undo_redo->add_do_method(ProjectSettings::get_singleton(), "set", name, va);
	undo_redo->add_undo_method(ProjectSettings::get_singleton(), "clear", name);
	undo_redo->add_do_method(this, "_update_actions");
	undo_redo->add_undo_method(this, "_update_actions");
	undo_redo->add_do_method(this, "_settings_changed");
	undo_redo->add_undo_method(this, "_settings_changed");
	undo_redo->commit_action();

	TreeItem *r = input_editor->get_root();

	if (!r)
		return;
	r = r->get_children();
	if (!r)
		return;
	while (r->get_next())
		r = r->get_next();

	if (!r)
		return;
	r->select(0);
	input_editor->ensure_cursor_is_visible();
}

void ProjectSettingsEditor::_item_checked(const String &p_item, bool p_check) {
}

void ProjectSettingsEditor::_save() {

	Error err = ProjectSettings::get_singleton()->save();
	message->set_text(err != OK ? TTR("Error saving settings.") : TTR("Settings saved OK."));
	message->popup_centered(Size2(300, 100) * EDSCALE);
}

void ProjectSettingsEditor::_settings_prop_edited(const String &p_name) {

	String full_item = globals_editor->get_full_item_path(p_name);

	_settings_changed();
}

void ProjectSettingsEditor::_settings_changed() {

	timer->start();
}

void ProjectSettingsEditor::queue_save() {
	_settings_changed();
}

void ProjectSettingsEditor::_copy_to_platform_about_to_show() {

	Set<String> presets;

	presets.insert("s3tc");
	presets.insert("etc");
	presets.insert("etc2");
	presets.insert("pvrtc");
	presets.insert("debug");
	presets.insert("release");

	for (int i = 0; i < EditorExport::get_singleton()->get_export_platform_count(); i++) {
		List<String> p;
		EditorExport::get_singleton()->get_export_platform(i)->get_platform_features(&p);
		for (List<String>::Element *E = p.front(); E; E = E->next()) {
			presets.insert(E->get());
		}
	}

	for (int i = 0; i < EditorExport::get_singleton()->get_export_preset_count(); i++) {

		List<String> p;
		EditorExport::get_singleton()->get_export_preset(i)->get_platform()->get_preset_features(EditorExport::get_singleton()->get_export_preset(i), &p);
		for (List<String>::Element *E = p.front(); E; E = E->next()) {
			presets.insert(E->get());
		}

		String custom = EditorExport::get_singleton()->get_export_preset(i)->get_custom_features();
		Vector<String> custom_list = custom.split(",");
		for (int i = 0; i < custom_list.size(); i++) {
			String f = custom_list[i].strip_edges();
			if (f != String()) {
				presets.insert(f);
			}
		}
	}

	popup_copy_to_feature->get_popup()->clear();
	int id = 0;
	for (Set<String>::Element *E = presets.front(); E; E = E->next()) {
		popup_copy_to_feature->get_popup()->add_item(E->get(), id++);
	}
}

void ProjectSettingsEditor::_copy_to_platform(int p_which) {

	String path = globals_editor->get_property_editor()->get_selected_path();
	if (path == String()) {
		EditorNode::get_singleton()->show_warning(TTR("Select an setting item first!"));
		return;
	}

	String property = globals_editor->get_current_section().plus_file(path);

	undo_redo->create_action(TTR("Override for Feature"));

	Variant value = ProjectSettings::get_singleton()->get(property);
	if (property.find(".") != -1) { //overwriting overwrite, keep overwrite
		undo_redo->add_do_method(ProjectSettings::get_singleton(), "clear", property);
		undo_redo->add_undo_method(ProjectSettings::get_singleton(), "set", property, value);
	}

	String feature = popup_copy_to_feature->get_popup()->get_item_text(p_which);
	String new_path = property + "." + feature;

	undo_redo->add_do_method(ProjectSettings::get_singleton(), "set", new_path, value);
	if (ProjectSettings::get_singleton()->has(new_path)) {
		undo_redo->add_undo_method(ProjectSettings::get_singleton(), "set", new_path, ProjectSettings::get_singleton()->get(new_path));
	}

	undo_redo->add_do_method(globals_editor, "update_category_list");
	undo_redo->add_undo_method(globals_editor, "update_category_list");

	undo_redo->add_do_method(this, "_settings_changed");
	undo_redo->add_undo_method(this, "_settings_changed");

	undo_redo->commit_action();
}

void ProjectSettingsEditor::add_translation(const String &p_translation) {

	_translation_add(p_translation);
}

void ProjectSettingsEditor::_translation_add(const String &p_path) {

	PoolStringArray translations = ProjectSettings::get_singleton()->get("locale/translations");

	for (int i = 0; i < translations.size(); i++) {

		if (translations[i] == p_path)
			return; //exists
	}

	translations.push_back(p_path);
	undo_redo->create_action(TTR("Add Translation"));
	undo_redo->add_do_property(ProjectSettings::get_singleton(), "locale/translations", translations);
	undo_redo->add_undo_property(ProjectSettings::get_singleton(), "locale/translations", ProjectSettings::get_singleton()->get("locale/translations"));
	undo_redo->add_do_method(this, "_update_translations");
	undo_redo->add_undo_method(this, "_update_translations");
	undo_redo->add_do_method(this, "_settings_changed");
	undo_redo->add_undo_method(this, "_settings_changed");
	undo_redo->commit_action();
}

void ProjectSettingsEditor::_translation_file_open() {

	translation_file_open->popup_centered_ratio();
}

void ProjectSettingsEditor::_translation_delete(Object *p_item, int p_column, int p_button) {

	TreeItem *ti = p_item->cast_to<TreeItem>();
	ERR_FAIL_COND(!ti);

	int idx = ti->get_metadata(0);

	PoolStringArray translations = ProjectSettings::get_singleton()->get("locale/translations");

	ERR_FAIL_INDEX(idx, translations.size());

	translations.remove(idx);

	undo_redo->create_action(TTR("Remove Translation"));
	undo_redo->add_do_property(ProjectSettings::get_singleton(), "locale/translations", translations);
	undo_redo->add_undo_property(ProjectSettings::get_singleton(), "locale/translations", ProjectSettings::get_singleton()->get("locale/translations"));
	undo_redo->add_do_method(this, "_update_translations");
	undo_redo->add_undo_method(this, "_update_translations");
	undo_redo->add_do_method(this, "_settings_changed");
	undo_redo->add_undo_method(this, "_settings_changed");
	undo_redo->commit_action();
}

void ProjectSettingsEditor::_translation_res_file_open() {

	translation_res_file_open->popup_centered_ratio();
}

void ProjectSettingsEditor::_translation_res_add(const String &p_path) {

	Variant prev;
	Dictionary remaps;

	if (ProjectSettings::get_singleton()->has("locale/translation_remaps")) {
		remaps = ProjectSettings::get_singleton()->get("locale/translation_remaps");
		prev = remaps;
	}

	if (remaps.has(p_path))
		return; //pointless already has it

	remaps[p_path] = PoolStringArray();

	undo_redo->create_action(TTR("Add Remapped Path"));
	undo_redo->add_do_property(ProjectSettings::get_singleton(), "locale/translation_remaps", remaps);
	undo_redo->add_undo_property(ProjectSettings::get_singleton(), "locale/translation_remaps", prev);
	undo_redo->add_do_method(this, "_update_translations");
	undo_redo->add_undo_method(this, "_update_translations");
	undo_redo->add_do_method(this, "_settings_changed");
	undo_redo->add_undo_method(this, "_settings_changed");
	undo_redo->commit_action();
}

void ProjectSettingsEditor::_translation_res_option_file_open() {

	translation_res_option_file_open->popup_centered_ratio();
}
void ProjectSettingsEditor::_translation_res_option_add(const String &p_path) {

	ERR_FAIL_COND(!ProjectSettings::get_singleton()->has("locale/translation_remaps"));

	Dictionary remaps = ProjectSettings::get_singleton()->get("locale/translation_remaps");

	TreeItem *k = translation_remap->get_selected();
	ERR_FAIL_COND(!k);

	String key = k->get_metadata(0);

	ERR_FAIL_COND(!remaps.has(key));
	PoolStringArray r = remaps[key];
	r.push_back(p_path + ":" + "en");
	remaps[key] = r;

	undo_redo->create_action(TTR("Resource Remap Add Remap"));
	undo_redo->add_do_property(ProjectSettings::get_singleton(), "locale/translation_remaps", remaps);
	undo_redo->add_undo_property(ProjectSettings::get_singleton(), "locale/translation_remaps", ProjectSettings::get_singleton()->get("locale/translation_remaps"));
	undo_redo->add_do_method(this, "_update_translations");
	undo_redo->add_undo_method(this, "_update_translations");
	undo_redo->add_do_method(this, "_settings_changed");
	undo_redo->add_undo_method(this, "_settings_changed");
	undo_redo->commit_action();
}

void ProjectSettingsEditor::_translation_res_select() {

	if (updating_translations)
		return;

	call_deferred("_update_translations");
}

void ProjectSettingsEditor::_translation_res_option_changed() {

	if (updating_translations)
		return;

	if (!ProjectSettings::get_singleton()->has("locale/translation_remaps"))
		return;

	Dictionary remaps = ProjectSettings::get_singleton()->get("locale/translation_remaps");

	TreeItem *k = translation_remap->get_selected();
	ERR_FAIL_COND(!k);
	TreeItem *ed = translation_remap_options->get_edited();
	ERR_FAIL_COND(!ed);

	String key = k->get_metadata(0);
	int idx = ed->get_metadata(0);
	String path = ed->get_metadata(1);
	int which = ed->get_range(1);

	Vector<String> langs = TranslationServer::get_all_locales();

	ERR_FAIL_INDEX(which, langs.size());

	ERR_FAIL_COND(!remaps.has(key));
	PoolStringArray r = remaps[key];
	ERR_FAIL_INDEX(idx, r.size());
	r.set(idx, path + ":" + langs[which]);
	remaps[key] = r;

	updating_translations = true;
	undo_redo->create_action(TTR("Change Resource Remap Language"));
	undo_redo->add_do_property(ProjectSettings::get_singleton(), "locale/translation_remaps", remaps);
	undo_redo->add_undo_property(ProjectSettings::get_singleton(), "locale/translation_remaps", ProjectSettings::get_singleton()->get("locale/translation_remaps"));
	undo_redo->add_do_method(this, "_update_translations");
	undo_redo->add_undo_method(this, "_update_translations");
	undo_redo->add_do_method(this, "_settings_changed");
	undo_redo->add_undo_method(this, "_settings_changed");
	undo_redo->commit_action();
	updating_translations = false;
}

void ProjectSettingsEditor::_translation_res_delete(Object *p_item, int p_column, int p_button) {

	if (updating_translations)
		return;

	if (!ProjectSettings::get_singleton()->has("locale/translation_remaps"))
		return;

	Dictionary remaps = ProjectSettings::get_singleton()->get("locale/translation_remaps");

	TreeItem *k = p_item->cast_to<TreeItem>();

	String key = k->get_metadata(0);
	ERR_FAIL_COND(!remaps.has(key));

	remaps.erase(key);

	undo_redo->create_action(TTR("Remove Resource Remap"));
	undo_redo->add_do_property(ProjectSettings::get_singleton(), "locale/translation_remaps", remaps);
	undo_redo->add_undo_property(ProjectSettings::get_singleton(), "locale/translation_remaps", ProjectSettings::get_singleton()->get("locale/translation_remaps"));
	undo_redo->add_do_method(this, "_update_translations");
	undo_redo->add_undo_method(this, "_update_translations");
	undo_redo->add_do_method(this, "_settings_changed");
	undo_redo->add_undo_method(this, "_settings_changed");
	undo_redo->commit_action();
}

void ProjectSettingsEditor::_translation_res_option_delete(Object *p_item, int p_column, int p_button) {

	if (updating_translations)
		return;

	if (!ProjectSettings::get_singleton()->has("locale/translation_remaps"))
		return;

	Dictionary remaps = ProjectSettings::get_singleton()->get("locale/translation_remaps");

	TreeItem *k = translation_remap->get_selected();
	ERR_FAIL_COND(!k);
	TreeItem *ed = p_item->cast_to<TreeItem>();
	ERR_FAIL_COND(!ed);

	String key = k->get_metadata(0);
	int idx = ed->get_metadata(0);

	ERR_FAIL_COND(!remaps.has(key));
	PoolStringArray r = remaps[key];
	ERR_FAIL_INDEX(idx, remaps.size());
	r.remove(idx);
	remaps[key] = r;

	undo_redo->create_action(TTR("Remove Resource Remap Option"));
	undo_redo->add_do_property(ProjectSettings::get_singleton(), "locale/translation_remaps", remaps);
	undo_redo->add_undo_property(ProjectSettings::get_singleton(), "locale/translation_remaps", ProjectSettings::get_singleton()->get("locale/translation_remaps"));
	undo_redo->add_do_method(this, "_update_translations");
	undo_redo->add_undo_method(this, "_update_translations");
	undo_redo->add_do_method(this, "_settings_changed");
	undo_redo->add_undo_method(this, "_settings_changed");
	undo_redo->commit_action();
}

void ProjectSettingsEditor::_update_translations() {

	//update translations

	if (updating_translations)
		return;

	updating_translations = true;

	translation_list->clear();
	TreeItem *root = translation_list->create_item(NULL);
	translation_list->set_hide_root(true);
	if (ProjectSettings::get_singleton()->has("locale/translations")) {

		PoolStringArray translations = ProjectSettings::get_singleton()->get("locale/translations");
		for (int i = 0; i < translations.size(); i++) {

			TreeItem *t = translation_list->create_item(root);
			t->set_editable(0, false);
			t->set_text(0, translations[i].replace_first("res://", ""));
			t->set_tooltip(0, translations[i]);
			t->set_metadata(0, i);
			t->add_button(0, get_icon("Del", "EditorIcons"), 0, false, TTR("Remove"));
		}
	}

	//update translation remaps

	String remap_selected;
	if (translation_remap->get_selected()) {
		remap_selected = translation_remap->get_selected()->get_metadata(0);
	}

	translation_remap->clear();
	translation_remap_options->clear();
	root = translation_remap->create_item(NULL);
	TreeItem *root2 = translation_remap_options->create_item(NULL);
	translation_remap->set_hide_root(true);
	translation_remap_options->set_hide_root(true);
	translation_res_option_add_button->set_disabled(true);

	Vector<String> langs = TranslationServer::get_all_locales();
	Vector<String> names = TranslationServer::get_all_locale_names();
	String langnames;
	for (int i = 0; i < names.size(); i++) {
		if (i > 0)
			langnames += ",";
		langnames += names[i];
	}

	if (ProjectSettings::get_singleton()->has("locale/translation_remaps")) {

		Dictionary remaps = ProjectSettings::get_singleton()->get("locale/translation_remaps");
		List<Variant> rk;
		remaps.get_key_list(&rk);
		Vector<String> keys;
		for (List<Variant>::Element *E = rk.front(); E; E = E->next()) {
			keys.push_back(E->get());
		}
		keys.sort();

		for (int i = 0; i < keys.size(); i++) {

			TreeItem *t = translation_remap->create_item(root);
			t->set_editable(0, false);
			t->set_text(0, keys[i].replace_first("res://", ""));
			t->set_tooltip(0, keys[i]);
			t->set_metadata(0, keys[i]);
			t->add_button(0, get_icon("Del", "EditorIcons"), 0, false, TTR("Remove"));
			if (keys[i] == remap_selected) {
				t->select(0);
				translation_res_option_add_button->set_disabled(false);

				PoolStringArray selected = remaps[keys[i]];
				for (int j = 0; j < selected.size(); j++) {

					String s = selected[j];
					int qp = s.find_last(":");
					String path = s.substr(0, qp);
					String locale = s.substr(qp + 1, s.length());

					TreeItem *t2 = translation_remap_options->create_item(root2);
					t2->set_editable(0, false);
					t2->set_text(0, path.replace_first("res://", ""));
					t2->set_tooltip(0, path);
					t2->set_metadata(0, j);
					t2->add_button(0, get_icon("Del", "EditorIcons"), 0, false, TTR("Remove"));
					t2->set_cell_mode(1, TreeItem::CELL_MODE_RANGE);
					t2->set_text(1, langnames);
					t2->set_editable(1, true);
					t2->set_metadata(1, path);
					int idx = langs.find(locale);
					print_line("find " + locale + " at " + itos(idx));
					if (idx < 0)
						idx = 0;

					t2->set_range(1, idx);
				}
			}
		}
	}

	updating_translations = false;
}

void ProjectSettingsEditor::_toggle_search_bar(bool p_pressed) {

	globals_editor->get_property_editor()->set_use_filter(p_pressed);

	if (p_pressed) {

		search_bar->show();
		add_prop_bar->hide();
		search_box->grab_focus();
		search_box->select_all();
	} else {

		search_bar->hide();
		add_prop_bar->show();
	}
}

void ProjectSettingsEditor::_clear_search_box() {

	if (search_box->get_text() == "")
		return;

	search_box->clear();
	globals_editor->get_property_editor()->update_tree();
}

void ProjectSettingsEditor::set_plugins_page() {

	tab_container->set_current_tab(plugin_settings->get_index());
}

void ProjectSettingsEditor::_bind_methods() {

	ClassDB::bind_method(D_METHOD("_item_selected"), &ProjectSettingsEditor::_item_selected);
	ClassDB::bind_method(D_METHOD("_item_add"), &ProjectSettingsEditor::_item_add);
	ClassDB::bind_method(D_METHOD("_item_adds"), &ProjectSettingsEditor::_item_adds);
	ClassDB::bind_method(D_METHOD("_item_del"), &ProjectSettingsEditor::_item_del);
	ClassDB::bind_method(D_METHOD("_item_checked"), &ProjectSettingsEditor::_item_checked);
	ClassDB::bind_method(D_METHOD("_save"), &ProjectSettingsEditor::_save);
	ClassDB::bind_method(D_METHOD("_action_add"), &ProjectSettingsEditor::_action_add);
	ClassDB::bind_method(D_METHOD("_action_adds"), &ProjectSettingsEditor::_action_adds);
	ClassDB::bind_method(D_METHOD("_action_selected"), &ProjectSettingsEditor::_action_selected);
	ClassDB::bind_method(D_METHOD("_action_edited"), &ProjectSettingsEditor::_action_edited);
	ClassDB::bind_method(D_METHOD("_action_activated"), &ProjectSettingsEditor::_action_activated);
	ClassDB::bind_method(D_METHOD("_action_button_pressed"), &ProjectSettingsEditor::_action_button_pressed);
	ClassDB::bind_method(D_METHOD("_update_actions"), &ProjectSettingsEditor::_update_actions);
	ClassDB::bind_method(D_METHOD("_wait_for_key"), &ProjectSettingsEditor::_wait_for_key);
	ClassDB::bind_method(D_METHOD("_add_item"), &ProjectSettingsEditor::_add_item, DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("_device_input_add"), &ProjectSettingsEditor::_device_input_add);
	ClassDB::bind_method(D_METHOD("_press_a_key_confirm"), &ProjectSettingsEditor::_press_a_key_confirm);
	ClassDB::bind_method(D_METHOD("_settings_prop_edited"), &ProjectSettingsEditor::_settings_prop_edited);
	ClassDB::bind_method(D_METHOD("_copy_to_platform"), &ProjectSettingsEditor::_copy_to_platform);
	ClassDB::bind_method(D_METHOD("_update_translations"), &ProjectSettingsEditor::_update_translations);
	ClassDB::bind_method(D_METHOD("_translation_delete"), &ProjectSettingsEditor::_translation_delete);
	ClassDB::bind_method(D_METHOD("_settings_changed"), &ProjectSettingsEditor::_settings_changed);
	ClassDB::bind_method(D_METHOD("_translation_add"), &ProjectSettingsEditor::_translation_add);
	ClassDB::bind_method(D_METHOD("_translation_file_open"), &ProjectSettingsEditor::_translation_file_open);

	ClassDB::bind_method(D_METHOD("_translation_res_add"), &ProjectSettingsEditor::_translation_res_add);
	ClassDB::bind_method(D_METHOD("_translation_res_file_open"), &ProjectSettingsEditor::_translation_res_file_open);
	ClassDB::bind_method(D_METHOD("_translation_res_option_add"), &ProjectSettingsEditor::_translation_res_option_add);
	ClassDB::bind_method(D_METHOD("_translation_res_option_file_open"), &ProjectSettingsEditor::_translation_res_option_file_open);
	ClassDB::bind_method(D_METHOD("_translation_res_select"), &ProjectSettingsEditor::_translation_res_select);
	ClassDB::bind_method(D_METHOD("_translation_res_option_changed"), &ProjectSettingsEditor::_translation_res_option_changed);
	ClassDB::bind_method(D_METHOD("_translation_res_delete"), &ProjectSettingsEditor::_translation_res_delete);
	ClassDB::bind_method(D_METHOD("_translation_res_option_delete"), &ProjectSettingsEditor::_translation_res_option_delete);

	ClassDB::bind_method(D_METHOD("_clear_search_box"), &ProjectSettingsEditor::_clear_search_box);
	ClassDB::bind_method(D_METHOD("_toggle_search_bar"), &ProjectSettingsEditor::_toggle_search_bar);

	ClassDB::bind_method(D_METHOD("_copy_to_platform_about_to_show"), &ProjectSettingsEditor::_copy_to_platform_about_to_show);
}

ProjectSettingsEditor::ProjectSettingsEditor(EditorData *p_data) {

	singleton = this;
	set_title(TTR("Project Settings (project.godot)"));
	set_resizable(true);
	undo_redo = &p_data->get_undo_redo();
	data = p_data;

	tab_container = memnew(TabContainer);
	tab_container->set_tab_align(TabContainer::ALIGN_LEFT);
	add_child(tab_container);
	//set_child_rect(tab_container);

	//tab_container->set_anchor_and_margin(MARGIN_LEFT,ANCHOR_BEGIN, 15 );
	//tab_container->set_anchor_and_margin(MARGIN_RIGHT,ANCHOR_END, 15 );
	//tab_container->set_anchor_and_margin(MARGIN_TOP,ANCHOR_BEGIN, 15 );
	//tab_container->set_anchor_and_margin(MARGIN_BOTTOM,ANCHOR_END, 35 );

	VBoxContainer *props_base = memnew(VBoxContainer);
	props_base->set_alignment(BoxContainer::ALIGN_BEGIN);
	props_base->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tab_container->add_child(props_base);
	props_base->set_name(TTR("General"));

	HBoxContainer *hbc = memnew(HBoxContainer);
	hbc->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	props_base->add_child(hbc);

	search_button = memnew(ToolButton);
	search_button->set_toggle_mode(true);
	search_button->set_pressed(false);
	search_button->set_text(TTR("Search"));
	hbc->add_child(search_button);
	search_button->connect("toggled", this, "_toggle_search_bar");

	hbc->add_child(memnew(VSeparator));

	add_prop_bar = memnew(HBoxContainer);
	add_prop_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hbc->add_child(add_prop_bar);

	Label *l = memnew(Label);
	add_prop_bar->add_child(l);
	l->set_text(TTR("Category:"));

	category = memnew(LineEdit);
	category->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_prop_bar->add_child(category);
	category->connect("text_entered", this, "_item_adds");

	l = memnew(Label);
	add_prop_bar->add_child(l);
	l->set_text(TTR("Property:"));

	property = memnew(LineEdit);
	property->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_prop_bar->add_child(property);
	property->connect("text_entered", this, "_item_adds");

	l = memnew(Label);
	add_prop_bar->add_child(l);
	l->set_text(TTR("Type:"));

	type = memnew(OptionButton);
	type->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_prop_bar->add_child(type);
	type->add_item("bool");
	type->add_item("int");
	type->add_item("float");
	type->add_item("string");

	Button *add = memnew(Button);
	add_prop_bar->add_child(add);
	add->set_text(TTR("Add"));
	add->connect("pressed", this, "_item_add");

	search_bar = memnew(HBoxContainer);
	search_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hbc->add_child(search_bar);
	search_bar->hide();

	search_box = memnew(LineEdit);
	search_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	search_bar->add_child(search_box);

	clear_button = memnew(ToolButton);
	search_bar->add_child(clear_button);
	clear_button->connect("pressed", this, "_clear_search_box");

	globals_editor = memnew(SectionedPropertyEditor);
	props_base->add_child(globals_editor);
	globals_editor->get_property_editor()->set_undo_redo(EditorNode::get_singleton()->get_undo_redo());
	globals_editor->get_property_editor()->set_property_selectable(true);
	//globals_editor->hide_top_label();
	globals_editor->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	globals_editor->register_search_box(search_box);
	globals_editor->get_property_editor()->get_scene_tree()->connect("cell_selected", this, "_item_selected");
	globals_editor->get_property_editor()->connect("property_toggled", this, "_item_checked", varray(), CONNECT_DEFERRED);
	globals_editor->get_property_editor()->connect("property_edited", this, "_settings_prop_edited");

	/*
	Button *save = memnew( Button );
	props_base->add_child(save);

	save->set_anchor(MARGIN_LEFT,ANCHOR_END);
	save->set_anchor(MARGIN_RIGHT,ANCHOR_END);
	save->set_anchor(MARGIN_TOP,ANCHOR_END);
	save->set_anchor(MARGIN_BOTTOM,ANCHOR_END);
	save->set_begin( Point2(80,28) );
	save->set_end( Point2(10,20) );
	save->set_text("Save");
	save->connect("pressed",this,"_save");
*/

	Button *del = memnew(Button);
	hbc->add_child(del);
	del->set_text(TTR("Delete"));
	del->connect("pressed", this, "_item_del");

	add_prop_bar->add_child(memnew(VSeparator));

	popup_copy_to_feature = memnew(MenuButton);
	popup_copy_to_feature->set_text(TTR("Override For.."));
	popup_copy_to_feature->set_disabled(true);
	add_prop_bar->add_child(popup_copy_to_feature);

	/*List<StringName> ep;
	EditorImportExport::get_singleton()->get_export_platforms(&ep);
	ep.sort_custom<StringName::AlphCompare>();

	for(List<StringName>::Element *E=ep.front();E;E=E->next()) {

		popup_copy_to_feature->get_popup()->add_item( E->get() );

	}*/

	popup_copy_to_feature->get_popup()->connect("id_pressed", this, "_copy_to_platform");
	popup_copy_to_feature->get_popup()->connect("about_to_show", this, "_copy_to_platform_about_to_show");

	get_ok()->set_text(TTR("Close"));
	set_hide_on_ok(true);

	message = memnew(ConfirmationDialog);
	add_child(message);
	//message->get_cancel()->hide();
	message->set_hide_on_ok(true);

	Control *input_base = memnew(Control);
	input_base->set_name(TTR("Input Map"));
	input_base->set_area_as_parent_rect();
	tab_container->add_child(input_base);

	VBoxContainer *vbc = memnew(VBoxContainer);
	input_base->add_child(vbc);
	vbc->set_anchor_and_margin(MARGIN_TOP, ANCHOR_BEGIN, 0);
	vbc->set_anchor_and_margin(MARGIN_BOTTOM, ANCHOR_END, 0);
	vbc->set_anchor_and_margin(MARGIN_LEFT, ANCHOR_BEGIN, 0);
	vbc->set_anchor_and_margin(MARGIN_RIGHT, ANCHOR_END, 0);

	l = memnew(Label);
	vbc->add_child(l);
	l->set_position(Point2(6, 5) * EDSCALE);
	l->set_text(TTR("Action:"));

	hbc = memnew(HBoxContainer);
	vbc->add_child(hbc);

	action_name = memnew(LineEdit);
	action_name->set_h_size_flags(SIZE_EXPAND_FILL);
	hbc->add_child(action_name);
	action_name->connect("text_entered", this, "_action_adds");

	add = memnew(Button);
	hbc->add_child(add);
	add->set_custom_minimum_size(Size2(150, 0) * EDSCALE);
	add->set_text(TTR("Add"));
	add->connect("pressed", this, "_action_add");

	input_editor = memnew(Tree);
	vbc->add_child(input_editor);
	input_editor->set_v_size_flags(SIZE_EXPAND_FILL);
	input_editor->connect("item_edited", this, "_action_edited");
	input_editor->connect("item_activated", this, "_action_activated");
	input_editor->connect("cell_selected", this, "_action_selected");
	input_editor->connect("button_pressed", this, "_action_button_pressed");
	popup_add = memnew(PopupMenu);
	add_child(popup_add);
	popup_add->connect("id_pressed", this, "_add_item");

	press_a_key = memnew(ConfirmationDialog);
	press_a_key->set_focus_mode(FOCUS_ALL);
	add_child(press_a_key);

	l = memnew(Label);
	l->set_text(TTR("Press a Key.."));
	l->set_area_as_parent_rect();
	l->set_align(Label::ALIGN_CENTER);
	l->set_margin(MARGIN_TOP, 20);
	l->set_anchor_and_margin(MARGIN_BOTTOM, ANCHOR_BEGIN, 30);
	press_a_key_label = l;
	press_a_key->add_child(l);
	press_a_key->connect("gui_input", this, "_wait_for_key");
	press_a_key->connect("confirmed", this, "_press_a_key_confirm");

	device_input = memnew(ConfirmationDialog);
	add_child(device_input);
	device_input->get_ok()->set_text(TTR("Add"));
	device_input->connect("confirmed", this, "_device_input_add");

	hbc = memnew(HBoxContainer);
	device_input->add_child(hbc);
	//device_input->set_child_rect(hbc);

	VBoxContainer *vbc_left = memnew(VBoxContainer);
	hbc->add_child(vbc_left);

	l = memnew(Label);
	l->set_text(TTR("Device:"));
	vbc_left->add_child(l);

	device_id = memnew(SpinBox);
	device_id->set_value(0);
	vbc_left->add_child(device_id);

	VBoxContainer *vbc_right = memnew(VBoxContainer);
	hbc->add_child(vbc_right);
	vbc_right->set_h_size_flags(SIZE_EXPAND_FILL);

	l = memnew(Label);
	l->set_text(TTR("Index:"));
	vbc_right->add_child(l);
	device_index_label = l;

	device_index = memnew(OptionButton);
	vbc_right->add_child(device_index);

	/*
	save = memnew( Button );
	input_base->add_child(save);
	save->set_anchor(MARGIN_LEFT,ANCHOR_END);
	save->set_anchor(MARGIN_RIGHT,ANCHOR_END);
	save->set_anchor(MARGIN_TOP,ANCHOR_END);
	save->set_anchor(MARGIN_BOTTOM,ANCHOR_END);
	save->set_begin( Point2(80,28) );
	save->set_end( Point2(10,20) );
	save->set_text("Save");
	save->connect("pressed",this,"_save");
*/
	setting = false;

	//translations
	TabContainer *translations = memnew(TabContainer);
	translations->add_style_override("panel", memnew(StyleBoxEmpty));
	translations->set_tab_align(TabContainer::ALIGN_LEFT);
	translations->set_name(TTR("Localization"));
	tab_container->add_child(translations);

	{

		VBoxContainer *tvb = memnew(VBoxContainer);
		translations->add_child(tvb);
		tvb->set_name(TTR("Translations"));
		HBoxContainer *thb = memnew(HBoxContainer);
		tvb->add_child(thb);
		thb->add_child(memnew(Label(TTR("Translations:"))));
		thb->add_spacer();
		Button *addtr = memnew(Button(TTR("Add..")));
		addtr->connect("pressed", this, "_translation_file_open");
		thb->add_child(addtr);
		VBoxContainer *tmc = memnew(VBoxContainer);
		tvb->add_child(tmc);
		tmc->set_v_size_flags(SIZE_EXPAND_FILL);
		translation_list = memnew(Tree);
		translation_list->set_v_size_flags(SIZE_EXPAND_FILL);
		tmc->add_child(translation_list);

		translation_file_open = memnew(EditorFileDialog);
		add_child(translation_file_open);
		translation_file_open->set_mode(EditorFileDialog::MODE_OPEN_FILE);
		translation_file_open->connect("file_selected", this, "_translation_add");
	}

	{
		VBoxContainer *tvb = memnew(VBoxContainer);
		translations->add_child(tvb);
		tvb->set_name(TTR("Remaps"));
		HBoxContainer *thb = memnew(HBoxContainer);
		tvb->add_child(thb);
		thb->add_child(memnew(Label(TTR("Resources:"))));
		thb->add_spacer();
		Button *addtr = memnew(Button(TTR("Add..")));
		addtr->connect("pressed", this, "_translation_res_file_open");
		thb->add_child(addtr);
		VBoxContainer *tmc = memnew(VBoxContainer);
		tvb->add_child(tmc);
		tmc->set_v_size_flags(SIZE_EXPAND_FILL);
		translation_remap = memnew(Tree);
		translation_remap->set_v_size_flags(SIZE_EXPAND_FILL);
		translation_remap->connect("cell_selected", this, "_translation_res_select");
		tmc->add_child(translation_remap);
		translation_remap->connect("button_pressed", this, "_translation_res_delete");

		translation_res_file_open = memnew(EditorFileDialog);
		add_child(translation_res_file_open);
		translation_res_file_open->set_mode(EditorFileDialog::MODE_OPEN_FILE);
		translation_res_file_open->connect("file_selected", this, "_translation_res_add");

		thb = memnew(HBoxContainer);
		tvb->add_child(thb);
		thb->add_child(memnew(Label(TTR("Remaps by Locale:"))));
		thb->add_spacer();
		addtr = memnew(Button(TTR("Add..")));
		addtr->connect("pressed", this, "_translation_res_option_file_open");
		translation_res_option_add_button = addtr;
		thb->add_child(addtr);
		tmc = memnew(VBoxContainer);
		tvb->add_child(tmc);
		tmc->set_v_size_flags(SIZE_EXPAND_FILL);
		translation_remap_options = memnew(Tree);
		translation_remap_options->set_v_size_flags(SIZE_EXPAND_FILL);
		tmc->add_child(translation_remap_options);

		translation_remap_options->set_columns(2);
		translation_remap_options->set_column_title(0, TTR("Path"));
		translation_remap_options->set_column_title(1, TTR("Locale"));
		translation_remap_options->set_column_titles_visible(true);
		translation_remap_options->set_column_expand(0, true);
		translation_remap_options->set_column_expand(1, false);
		translation_remap_options->set_column_min_width(1, 200);
		translation_remap_options->connect("item_edited", this, "_translation_res_option_changed");
		translation_remap_options->connect("button_pressed", this, "_translation_res_option_delete");

		translation_res_option_file_open = memnew(EditorFileDialog);
		add_child(translation_res_option_file_open);
		translation_res_option_file_open->set_mode(EditorFileDialog::MODE_OPEN_FILE);
		translation_res_option_file_open->connect("file_selected", this, "_translation_res_option_add");
	}

	{
		autoload_settings = memnew(EditorAutoloadSettings);
		autoload_settings->set_name(TTR("AutoLoad"));
		tab_container->add_child(autoload_settings);
		autoload_settings->connect("autoload_changed", this, "_settings_changed");
	}

	{

		plugin_settings = memnew(EditorPluginSettings);
		plugin_settings->set_name(TTR("Plugins"));
		tab_container->add_child(plugin_settings);
	}

	timer = memnew(Timer);
	timer->set_wait_time(1.5);
	timer->connect("timeout", ProjectSettings::get_singleton(), "save");
	timer->set_one_shot(true);
	add_child(timer);

	updating_translations = false;

	/*
	Control * es = memnew( Control );
	es->set_name("Export");
	tab_container->add_child(es);
	export_settings = memnew( ProjectExportSettings );
	es->add_child(export_settings);
	export_settings->set_area_as_parent_rect();
	export_settings->set_anchor_and_margin(MARGIN_BOTTOM,ANCHOR_END, 35 );
*/
}
