architecture rtl of top is
begin
  y <= a when sel = '1' else b;
end;
