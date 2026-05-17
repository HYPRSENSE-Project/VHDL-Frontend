architecture rtl of top is
begin
process(a, b, sel)
begin
  if sel = '1' then
    y <= a;
  else
    y <= b;
  end if;
end process;
end;
