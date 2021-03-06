M.scene_map_file = nil
M.save_scenes = function()
   local mapfile = io.open(M.scene_map_file, "w")
   mapfile:write("local map = {}\n")
   for k, v in spairs(scenes_map) do
      if k ~= "" then
         mapfile:write(("map['%s'] = '%s'\n"):format(k, v))
      end
   end
   mapfile:write("return map\n")
   mapfile:close()
end

M.load_scene_map = function()
   if not M.scene_map_file then
      local tmp_scene_map_file = M.configDirFile(("scenes-map-%dx%d@%s.lua"):format(M.real_width, M.real_height, android_serial))
      dofile_res, M.scenes_map = pcall(dofile, tmp_scene_map_file)
      if not dofile_res then
         M.scenes_map = {}
      else
         M.scene_map_file = tmp_scene_map_file
      end
   end
end

M.forget_scene = function(scene)
   log("Forget scene: %s", scene)
   M.scenes_map[scene] = nil
end

M.refind_scene = function(scene, retry)
   if not retry or retry < 1 then
      retry = 1
   else
      retry = retry + 1
   end

   for i = 1, retry do
      if find_scene(scene) then
         return true
      end
      sleep(.1)
   end

   if retry == 1 then
      return false
   end

   forget_scene(scene)
   return find_scene(scene)
end

M.find_scene = function(scene, times)
   load_scene_map()
   if not times then
      times = 1
   end
   if not M.scenes_map[scene] then
      for i = 1, times do
         sleep(1)
         scene_xy = qx("find-scene.sh find-scene -s " .. scene .. " --scene-dir " .. M.resDir)
         if scene_xy ~= "" then
            break
         elseif i == times then
            return nil
         end
      end
      M.scenes_map[scene] = scene_xy
      save_scenes()
   end
   scene_xy = M.scenes_map[scene]

   s_x = scene_xy:gsub(" .*", "")
   s_y = scene_xy:gsub(".* ", "")
   if system(("find-scene.sh is-scene -x %s -y %s -s %s"):format(s_x, s_y, scene)) then
      log("found scene: %s at %s %s", scene, s_x, s_y)
      return true
   else
      log("! found scene: %s at %s %s", scene, s_x, s_y)
      return false
   end
end

M.click_scene = function (scene, retry, xy_tab)
   log("Click scene: %s", scene)
   if not xy_tab then
      xy_tab = { }
   end

   if not refind_scene(scene, retry) then
      log("Can't find scene: %s for click (retry: %s)", scene, retry)
      return
   end
   local xy = scenes_map[scene]
   xy = split(" ", xy)
   adb_tap_XY(xy[1] + (xy_tab.x or 0), xy[2] + (xy_tab.y or 0))
end

