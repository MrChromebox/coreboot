-- SPDX-License-Identifier: GPL-2.0-or-later

with HW.GFX.GMA;
with HW.GFX.GMA.Display_Probing;

use HW.GFX.GMA;
use HW.GFX.GMA.Display_Probing;

private package GMA.Mainboard is

   -- DDI B goes to an LSPCON, DDI C to an MST hub. Their DDC pins
   -- aren't connected, so only probe them as DP.
   ports : constant Port_List :=
     (DP1,
      DP2,
      others => Disabled);

end GMA.Mainboard;
