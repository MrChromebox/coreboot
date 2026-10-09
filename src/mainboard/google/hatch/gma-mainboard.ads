-- SPDX-License-Identifier: GPL-2.0-or-later

with HW.GFX.GMA;
with HW.GFX.GMA.Display_Probing;

use HW.GFX.GMA;
use HW.GFX.GMA.Display_Probing;

private package GMA.Mainboard is

   -- DDI B/C only go to the USB-C ports, their DDC pins aren't connected
   ports : constant Port_List :=
     (eDP,
      DP1,
      DP2,
      others => Disabled);

end GMA.Mainboard;
