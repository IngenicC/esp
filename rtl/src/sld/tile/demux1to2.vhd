library ieee;
use ieee.std_logic_1164.all;


  entity demux1to2 is
    port(
    data_in: in std_logic;
    sel: in std_logic;
    out1,out2: out std_logic);
  end demux1to2;

  architecture arch of demux1to2 is
  begin
    process(data_in,sel)
    begin
      if  sel='1' then
        out1<=data_in;
      else
        out2<=data_in;
      end if;
    end process;
  end arch;





  
