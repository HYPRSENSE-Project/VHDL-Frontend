library ieee;
use ieee.std_logic_1164.all;

entity top is
  port(a : in std_logic; b : out std_logic);
end;

architecture rtl of top is
begin
  b <= a;
end;
