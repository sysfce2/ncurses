------------------------------------------------------------------------------
--                                                                          --
--                       GNAT ncurses Binding Samples                       --
--                                                                          --
--                          Sample.Menu_Demo.Handler                        --
--                                                                          --
--                                 B O D Y                                  --
--                                                                          --
------------------------------------------------------------------------------
-- Copyright (c) 1998 Free Software Foundation, Inc.                        --
--                                                                          --
-- Permission is hereby granted, free of charge, to any person obtaining a  --
-- copy of this software and associated documentation files (the            --
-- "Software"), to deal in the Software without restriction, including      --
-- without limitation the rights to use, copy, modify, merge, publish,      --
-- distribute, distribute with modifications, sublicense, and/or sell       --
-- copies of the Software, and to permit persons to whom the Software is    --
-- furnished to do so, subject to the following conditions:                 --
--                                                                          --
-- The above copyright notice and this permission notice shall be included  --
-- in all copies or substantial portions of the Software.                   --
--                                                                          --
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  --
-- OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               --
-- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   --
-- IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   --
-- DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    --
-- OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    --
-- THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               --
--                                                                          --
-- Except as contained in this notice, the name(s) of the above copyright   --
-- holders shall not be used in advertising or otherwise to promote the     --
-- sale, use or other dealings in this Software without prior written       --
-- authorization.                                                           --
------------------------------------------------------------------------------
--  Author: Juergen Pfeifer <Juergen.Pfeifer@T-Online.de> 1996
--  Version Control
--  $Revision: 1.4 $
------------------------------------------------------------------------------
with Sample.Menu_Demo.Aux;
with Sample.Explanation; use Sample.Explanation;
with Sample.Manifest; use Sample.Manifest;

package body Sample.Menu_Demo.Handler is

   package Aux renames Sample.Menu_Demo.Aux;

   procedure Drive_Me (M     : in Menu;
                       Title : in String := "")
   is
      L : Line_Count;
      C : Column_Count;
      Y : Line_Position;
      X : Column_Position;
   begin
      Aux.Geometry (M, L, C, Y, X);
      Drive_Me (M, Y, X, Title);
   end Drive_Me;

   procedure Drive_Me (M     : in Menu;
                       Lin   : in Line_Position;
                       Col   : in Column_Position;
                       Title : in String := "")
   is
      Pan : Panel := Aux.Create (M, Title, Lin, Col);
      V   : Cursor_Visibility := Invisible;
   begin
      Set_Cursor_Visibility (V);
      loop
         declare
            K : Key_Code := Aux.Get_Request (M, Pan);
            R : Driver_Result := Driver (M, K);
         begin
            case R is
               when Menu_Ok => null;
               when Unknown_Request =>
                  declare
                     I : constant Item := Current (M);
                     O : Item_Option_Set;
                  begin
                     Get_Options (I, O);
                     if K = SELECT_ITEM and then not O.Selectable then
                        Beep;
                     else
                        if My_Driver (M, K, Pan) then
                           exit;
                        end if;
                     end if;
                  end;
               when others => Beep;
            end case;
         end;
      end loop;
      Set_Cursor_Visibility (V);
      Aux.Destroy (M, Pan);
   end Drive_Me;

end Sample.Menu_Demo.Handler;
