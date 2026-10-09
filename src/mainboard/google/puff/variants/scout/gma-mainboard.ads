-- SPDX-License-Identifier: GPL-2.0-or-later

with HW.GFX.GMA;
with HW.GFX.GMA.Display_Probing;

use HW.GFX.GMA;
use HW.GFX.GMA.Display_Probing;

private package GMA.Mainboard is

   -- Only DDI B has its DDC pins connected
   ports : constant Port_List :=
     (HDMI1,
      DP2,
      others => Disabled);

end GMA.Mainboard;
