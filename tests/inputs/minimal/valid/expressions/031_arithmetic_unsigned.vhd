architecture rtl of top is
signal a, b : unsigned(7 downto 0);
signal y : unsigned(7 downto 0);
begin

y <= a + b;
end;