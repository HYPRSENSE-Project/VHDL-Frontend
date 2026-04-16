architecture rtl of top is
begin
process(clk)
begin
  if rising_edge(clk) then
    q <= d;
  end if;
end process;
end;