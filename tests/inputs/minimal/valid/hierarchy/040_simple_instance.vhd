architecture rtl of top is
begin
u1: entity work.child
  port map (
    a => a,
    b => b
  );
  end;