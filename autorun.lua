local_index = nil
mouse_position = nil
screen_size = nil
Tahoma = surface.create_font("Tahoma", 12, 500, 0, 128)
oPunch = c_vector.new();oPunch.x=0;oPunch.y=0;oPunch.z=0;

function rcs_hack()
	if(local_index ~= nil) then
		local local_entity = entity_list.get_entity(local_index)
		local local_view_angles = engine.get_view_angles()
		if(local_entity ~= nil) then
			local iShotsFired = entity.get_prop(local_entity,"CCSPlayer","m_iShotsFired",0)
			local aimPunchAngle = entity.get_prop(local_entity,"CBasePlayer","m_aimPunchAngle",0)
			local punchAngle = c_vector.new()
			if(aimPunchAngle ~= nil) then
				punchAngle.x = aimPunchAngle.x * 1.85
				punchAngle.y = aimPunchAngle.y * 1.85
				if(iShotsFired ~= nil and iShotsFired > 1) then
					local newAngle = c_vector.new()
					newAngle.x = local_view_angles.x + oPunch.x - punchAngle.x;
					newAngle.y = local_view_angles.y + oPunch.y - punchAngle.y;
					engine.set_view_angles(newAngle);
				end
			end
			oPunch.x = punchAngle.x;
			oPunch.y = punchAngle.y;
		end
	end
end

function radar_hack()
	if(local_index ~= nil) then
		for i=2,64 do
			local entity_tmp = entity_list.get_entity(i)
			if(entity_tmp ~= nil) then
				entity.set_netvar_bool(entity_tmp,"CBaseEntity","m_bSpotted",0,true)
			end
		end
	end
end

client.set_event_callback("on_load", function()
	--print("lua_callback_on_load")
end)

client.set_event_callback("on_unload", function()
	--print("lua_callback_on_unload")
end)

client.set_event_callback("on_frame_stage_notify", function(stage)
	--print("lua_callback_on_frame_stage_notify")
end)

client.set_event_callback("on_create_move", function(cmd)
	--print("lua_callback_on_create_move")
	if(engine.is_in_game() and engine.is_connected()) then
		local_index = engine.get_local_player_index()
	else
		local_index = nil
	end
	
	--if(input_system.is_key_down(KEYCODES_C.VK_LBUTTON)) then
		--aimlock(cmd.viewangles)
	--end
	
	if(input_system.is_key_down(KEYCODES_B.VK_F)) then
		radar_hack()
	end
	
	rcs_hack()
end)

client.set_event_callback("on_paint", function()
	--print("lua_callback_on_paint")
	--surface.set_draw_color(255,255,255,255)
	--surface.draw_line(0,0,100,0)
	--surface.draw_line(0,0,0,100)
	--surface.draw_line(100,100,100,0)
	--surface.draw_line(100,100,0,100)
	--surface.draw_line(0,0,100,100)
	
	surface.set_text_color(255,0,0,255)
	surface.set_text_font(Tahoma)
	surface.set_text_pos(0,200)
	screen_size = surface.get_screen_size()
	mouse_position = input_system.get_mouse_position()
	surface.draw_text("x:"..mouse_position.x)
	
	surface.set_text_pos(0,215)
	surface.draw_text("y:"..mouse_position.y)
	--surface.draw_filled_rect_fade(100,100,200,200,255,100,true)
end)

client.set_event_callback("on_test", function()
	print("lua_callback_on_test")
	--local cl = c_color.new(0,255,255,255)
	--cvar.console_color_printf(cl,"\nfuck\n")
	--sv_cheats = cvar.find_var("sv_cheats")
	--convar.set_int(sv_cheats,1)
	
	--r_drawothermodels = cvar.find_var("r_drawothermodels")
	--convar.set_int(r_drawothermodels,2)
	
	--cl_grenadepreview = cvar.find_var("cl_grenadepreview")
	--convar.set_int(cl_grenadepreview,1)
	
	--bot_stop = cvar.find_var("bot_stop")
	--convar.set_int(bot_stop,1)
	
	--sv_infinite_ammo = cvar.find_var("sv_infinite_ammo")
	--convar.set_int(sv_infinite_ammo,1)
	
	--sv_autobunnyhopping = cvar.find_var("sv_autobunnyhopping")
	--convar.set_int(sv_autobunnyhopping,1)
end)

--client.refresh()