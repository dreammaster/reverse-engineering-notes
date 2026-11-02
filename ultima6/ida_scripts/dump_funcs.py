import idc, idautils, ida_funcs
names = ["use_food","use_potion","use_scroll","use_key_or_lockpick","use_torch","use_fan","use_door","use_key_on_door","search_here","use_fountain","use_gems","use_sextant","use_shovel","bury_object","use_rubber_ducky","use_beehive","ring_bell","use_churn","fill_bucket_milk","fill_bucket_water","use_fishing_pole","use_silver_horn","operate_drawbridge","use_crystal_ball","use_telescope","use_shrine","use_orb_of_the_moons","use_cannon","light_powder_keg","extinguish_light","play_instrument","operate_passthrough","open_secret_door","use_vortex_cube","climb_ladder","use_lever","use_switch"]
out=open(r"C:\dev\ultima\ultima6\u6\use_funcs.txt","w")
for n in names:
    ea = idc.get_name_ea_simple(n)
    out.write("=== %s %05X\n"%(n, ea if ea!=idc.BADADDR else 0))
    if ea==idc.BADADDR: continue
    f = ida_funcs.get_func(ea)
    if not f: continue
    cur=f.start_ea
    while cur<f.end_ea:
        if not idc.is_code(idc.get_full_flags(cur)):
            idc.del_items(cur,0,4); k=idc.create_insn(cur) or 1
        else: k=idc.get_item_size(cur)
        out.write("%05X %s\n"%(cur, idc.generate_disasm_line(cur,0)))
        cur+=k
out.close()
