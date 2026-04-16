architecture rtl of top is
begin
process(clk)
begin
  if rising_edge(clk) then
    if rst = '1' then
      q <= '0';
    else
      q <= d;
    end if;
  end if;
end process;
end;