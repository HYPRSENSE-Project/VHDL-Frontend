entity top is
  port(a : in std_logic; b : out std_logic);
end;

architecture rtl of top is
begin
  b <= a;
end;
