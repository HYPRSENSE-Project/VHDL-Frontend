architecture rtl of top is
begin
process(clk)
begin
  if rising_edge(clk) then
    r1 <= a;
    r2 <= r1;
  end if;
end process;
end;