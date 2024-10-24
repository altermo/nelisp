local bytes=require'nelisp.bytes'
local signal=require'nelisp.signal'
local M={}

---@overload fun(c:number):boolean
function M.charbyte8p(c) return bytes.MAX_5_BYTE_CHAR<c end
---@overload fun(c:number):boolean
function M.asciicharp(c) return 0<=c and c<0x80 end
---@overload fun(c:number):boolean
function M.charbyte8headp(c) return c==0xc0 or c==0xc1 end
---@overload fun(c:number):boolean
function M.singlebytecharp(c) return 0<=c and c<0x100 end

---@param c number
---@return string
function M.byte8string(c)
    return string.char(bit.bor(0xc0,bit.band(bit.rshift(c,6),0x01)),bit.bor(0x80,bit.band(c,0x3f)))
end
---@param c number
---@return string
function M.charstring(c)
    assert(0<=c)
    if bit.band(c,bytes.CHAR_MODIFIER_MASK)>0 then
        error('TODO')
    end
    if c<=bytes.MAX_1_BYTE_CHAR then
        return string.char(c)
    elseif c<=bytes.MAX_2_BYTE_CHAR then
        return string.char(bit.bor(0xc0,bit.rshift(c,6)),bit.bor(0x80,bit.band(c,0x3f)))
    elseif c<=bytes.MAX_3_BYTE_CHAR then
        error('TODO')
    elseif c<=bytes.MAX_4_BYTE_CHAR then
        error('TODO')
    elseif c<=bytes.MAX_5_BYTE_CHAR then
        error('TODO')
    elseif c<=bytes.MAX_CHAR then
        c=M.chartobyte8(c)
        return M.byte8string(c)
    else
        signal.error('Invalid character: %x',c)
        error('unreachable')
    end
end
---@param s string
---@return string
function M.strasunibyte(s)
    local out=''
    local p=1
    while p<#s do
        local c=string.byte(s:sub(p,p))
        local len=M.bytesbycharhead(c)
        if M.charbyte8headp(c) then
            len,c=M.stringcharandlength(s:sub(p))
            p=p+len
            out=out..string.char(M.chartobyte8(c))
        else
            for _=1,len do
                out=out..s:sub(p,p)
                p=p+1
            end
        end
    end
    return out
end
---@param s string
---@return number
---@return number
function M.stringcharandlength(s)
    local c,p1,p2,p3,p4=string.byte(s,1,5)
    assert(c)
    if bit.band(c,0x80)==0 then
        return 1,c
    end
    assert(0xc0<=c and p1)
    local d=bit.lshift(c,6)+p1-(bit.lshift(0xc0,6)+0x80)
    if bit.band(c,0x20)==0 then
        return 2,d+(c<0xc2 and 0x3fff80 or 0)
    end
    error('TODO')
    assert(p2)
    assert(p3)
    assert(p4)
end

---@param c number
function M.chartobyte8(c)
    return M.charbyte8p(c) and c-0x3fff00 or bit.band(c,0xff)
end
---@param c number
function M.byte8tochar(c)
    return c+0x3fff00
end
---@param c number
---@return number
function M.bytesbycharhead(c)
    return (bit.band(c,0x80)==0 and 1) or
        (bit.band(c,0x20)==0 and 2) or
        (bit.band(c,0x10)==0 and 3) or
        (bit.band(c,0x08)==0 and 4) or 5
end
---@param c number
---@return number
function M.charhexdigit(c)
    return ({
        [bytes'0']=0,[bytes'1']=1,[bytes'2']=2,[bytes'3']=3,
        [bytes'4']=4,[bytes'5']=5,[bytes'6']=6,[bytes'7']=7,
        [bytes'8']=8,[bytes'9']=9,[bytes'a']=10,[bytes'b']=11,
        [bytes'c']=12,[bytes'd']=13,[bytes'e']=14,[bytes'f']=15,
        [bytes'A']=10,[bytes'B']=11,[bytes'C']=12,[bytes'D']=13,
        [bytes'E']=14,[bytes'F']=15,
    })[c] or -1
end

M.max_int=0x1fffffffffffff

return M
