local L0_1, L1_1

function L0_1()
  local L0_2, L1_2, L2_2, L3_2, L4_2, L5_2, L6_2, L7_2, L8_2, L9_2, L10_2, L11_2, L12_2, L13_2, L14_2, L15_2
  L0_2 = toHex
  L1_2 = managerOffsetList
  L1_2 = L1_2.TeamManager
  L0_2 = L0_2(L1_2)
  L1_2 = readPointer
  L2_2 = "[[[GameEngine]+DE8]+F0]+"
  L3_2 = L0_2
  L2_2 = L2_2 .. L3_2
  L1_2 = L1_2(L2_2)
  L2_2 = readPointer
  L3_2 = L1_2
  L2_2 = L2_2(L3_2)
  if L2_2 then
    L2_2 = gfo
    L3_2 = L1_2
    L2_2 = L2_2(L3_2)
    L3_2 = readInteger
    L4_2 = "[[[[GameEngine]+DE8]+F0]+"
    L5_2 = L0_2
    L6_2 = "]+8+"
    L7_2 = toHex
    L8_2 = L2_2.TeamInfos
    L7_2 = L7_2(L8_2)
    L4_2 = L4_2 .. L5_2 .. L6_2 .. L7_2
    L3_2 = L3_2(L4_2)
    if not L3_2 then
      L3_2 = 0
    end
    L4_2 = 0
    L5_2 = L3_2 - 1
    L6_2 = 1
    for L7_2 = L4_2, L5_2, L6_2 do
      L8_2 = readPointer
      L9_2 = "[[[[[[GameEngine]+DE8]+F0]+"
      L10_2 = L0_2
      L11_2 = "]+"
      L12_2 = toHex
      L13_2 = L2_2.TeamInfos
      L12_2 = L12_2(L13_2)
      L13_2 = "]+"
      L14_2 = toHex
      L15_2 = L7_2 * 8
      L14_2 = L14_2(L15_2)
      L15_2 = "]+[Stack+158]"
      L9_2 = L9_2 .. L10_2 .. L11_2 .. L12_2 .. L13_2 .. L14_2 .. L15_2
      L8_2 = L8_2(L9_2)
      L9_2 = readFloat
      L10_2 = L8_2 + 76
      L9_2 = L9_2(L10_2)
      L10_2 = readFloat
      L11_2 = L8_2 + 108
      L10_2 = L10_2(L11_2)
      L11_2 = writeFloat
      L12_2 = L8_2 + 56
      L13_2 = L9_2
      L11_2(L12_2, L13_2)
      L11_2 = writeFloat
      L12_2 = L8_2 + 60
      L13_2 = L9_2
      L11_2(L12_2, L13_2)
      L11_2 = writeFloat
      L12_2 = L8_2 + 88
      L13_2 = L10_2
      L11_2(L12_2, L13_2)
      L11_2 = writeFloat
      L12_2 = L8_2 + 440
      L13_2 = 1000
      L11_2(L12_2, L13_2)
      L11_2 = writeFloat
      L12_2 = L8_2 + 444
      L13_2 = 1000
      L11_2(L12_2, L13_2)
    end
  end
end

recoverTeam = L0_1

