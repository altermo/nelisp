local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local signal=require'nelisp.signal'
local b=require'nelisp.bytes'
local alloc=require'nelisp.alloc'
local lread=require'nelisp.lread'
local fns=require'nelisp.fns'
local overflow=require'nelisp.overflow'
local chartab=require'nelisp.chartab'

---@enum nelisp.charset_method
local charset_method={
    offset=0,
    map=1,
    subset=2,
    superset=3,
}

---@class nelisp.charset
---@field id number
---@field hash_index number
---@field dimension number
---@field code_space number[][15] (0-indexed)
---@field code_space_mask number[][256] (0-indexed)
---@field code_linear_p boolean
---@field iso_chars_96 boolean
---@field ascii_compatible_p boolean
---@field supplementary_p boolean
---@field compact_codes_p boolean
---@field unified_p boolean
---@field iso_final number
---@field iso_revision number
---@field emacs_mule_id number
---@field method nelisp.charset_method
---@field min_code number
---@field max_code number
---@field char_index_offset number
---@field min_char number
---@field max_char number
---@field invalid_code number
---@field fast_map number[][190] (0-indexed)
---@field code_offset number

---@enum nelisp.charset_idx
local charset_idx={
    id=0,
    name=1,
    plist=2,
    map=3,
    decoder=4,
    encoder=5,
    subset=6,
    superset=7,
    unify_map=8,
    deunifier=9,
    max=10
}
---@enum nelisp.charset_arg
local charset_arg={
    name=1,
    dimension=2,
    code_space=3,
    min_code=4,
    max_code=5,
    iso_final=6,
    iso_revision=7,
    emacs_mule_id=8,
    ascii_compatible_p=9,
    supplementary_p=10,
    invalid_code=11,
    code_offset=12,
    map=13,
    subset=14,
    superset=15,
    unify_map=16,
    plist=17,
    max=17,
}

local M={}
local F={}
local function charset_symbol_attributes(sym)
    return vars.F.gethash(sym,vars.charset_hash_table,vars.Qnil)
end
local function charset_attributes(charset)
    return lisp.aref((vars.charset_hash_table --[[@as nelisp._hash_table]]).key_and_value,charset.hash_index*2+1)
end
---@param cs nelisp.charset
---@return nelisp.obj
function M.charset_name(cs)
    return lisp.aref(charset_attributes(cs),charset_idx.name)
end
---@param name nelisp.obj
---@param dimension number
---@param code_space_chars string
---@param min_code number
---@param max_code number
---@param iso_final number
---@param iso_revision number
---@param emacs_mule_id number
---@param ascii_compatible boolean
---@param supplementary boolean
---@param code_offset number
---@return number
local function define_charset_internal(name,dimension,code_space_chars,min_code,max_code,iso_final,iso_revision,emacs_mule_id,ascii_compatible,supplementary,code_offset)
    local code_space_vector=alloc.make_vector(8,'nil')
    local i=0
    for c in (code_space_chars..'\0'):gmatch'.' do
        lisp.aset(code_space_vector,i,lisp.make_fixnum(string.byte(c)))
        i=i+1
        if i==8 then break end
    end
    ---@type nelisp.obj[]
    local args={}
    args[charset_arg.name]=name
    args[charset_arg.dimension]=lisp.make_fixnum(dimension)
    args[charset_arg.code_space]=code_space_vector
    args[charset_arg.min_code]=lisp.make_fixnum(min_code)
    args[charset_arg.max_code]=lisp.make_fixnum(max_code)
    args[charset_arg.iso_final]=(iso_final<0 and vars.Qnil or lisp.make_fixnum(iso_final))
    args[charset_arg.iso_revision]=lisp.make_fixnum(iso_revision)
    args[charset_arg.emacs_mule_id]=(emacs_mule_id<0 and vars.Qnil or lisp.make_fixnum(emacs_mule_id))
    args[charset_arg.ascii_compatible_p]=ascii_compatible and vars.Qt or vars.Qnil
    args[charset_arg.supplementary_p]=supplementary and vars.Qt or vars.Qnil
    args[charset_arg.invalid_code]=vars.Qnil
    args[charset_arg.code_offset]=lisp.make_fixnum(code_offset)
    args[charset_arg.map]=vars.Qnil
    args[charset_arg.subset]=vars.Qnil
    args[charset_arg.superset]=vars.Qnil
    args[charset_arg.unify_map]=vars.Qnil
    args[charset_arg.plist]=lisp.list(
        vars.QCname,
        args[charset_arg.name],
        lread.intern_c_string(':dimension'),
        args[charset_arg.dimension],
        lread.intern_c_string(':code-space'),
        args[charset_arg.code_space],
        lread.intern_c_string(':iso-final-char'),
        args[charset_arg.iso_final],
        lread.intern_c_string(':emacs-mule-id'),
        args[charset_arg.emacs_mule_id],
        vars.QCascii_compatible_p,
        args[charset_arg.ascii_compatible_p],
        lread.intern_c_string(':code-offset'),
        args[charset_arg.code_offset])
    vars.F.define_charset_internal(args)
    return lisp.fixnum(lisp.aref(charset_symbol_attributes(name),charset_idx.id))
end
local function code_point_to_index(charset,code)
    return charset.code_linear_p and (code-charset.min_code) or (
        (bit.band(charset.code_space_mask[bit.rshift(code,24)],0x8)>0
        and bit.band(charset.code_space_mask[bit.band(bit.rshift(code,16),0xff)],0x4)>0
        and bit.band(charset.code_space_mask[bit.band(bit.rshift(code,8),0xff)],0x2)>0
        and bit.band(charset.code_space_mask[bit.band(code,0xff)],0x1)>0) and (
        ((bit.rshift(code,24)-charset.code_space[12])
        *charset.code_space[11])
        +((bit.band(bit.rshift(code,16),0xff)-charset.code_space[8])
        *charset.code_space[7])
        +((bit.band(bit.rshift(code,8),0xff)-charset.code_space[4])
        *charset.code_space[3])
        +(bit.band(code,0xff)-charset.code_space[0])
        -charset.char_index_offset
    ) or -1)
end
local function charset_fast_map_set(c,fast_map)
    if c<0x10000 then
        fast_map[bit.rshift(c,10)]=bit.bor(fast_map[bit.rshift(c,10)],bit.lshift(1,bit.band(bit.rshift(c,7),7)))
    else
        fast_map[bit.rshift(c,15)]=bit.bor(fast_map[bit.rshift(c,15)+62],bit.lshift(1,bit.band(bit.rshift(c,12),7)))
    end
end
---@param charset nelisp.charset
---@param control_flag number
local function load_charset(charset,control_flag)
    local map
    if charset.method==charset_method.map then
        map=lisp.aref(charset_attributes(charset),charset_idx.map)
    else
        error('TODO')
    end
    if not _G.nelisp_later then
        error('TODO')
    else
        charset.min_char=0
        charset.max_char=0
    end
end
local function check_charset_get_id(x)
    if not lisp.symbolp(x) then
        signal.wrong_type_argument(vars.Qcharsetp,x)
    end
    local idx=fns.hash_lookup((vars.charset_hash_table --[[@as nelisp._hash_table]]),x)
    if idx<0 then
        signal.wrong_type_argument(vars.Qcharsetp,x)
    end
    return lisp.fixnum(lisp.aref(lisp.aref((vars.charset_hash_table --[[@as nelisp._hash_table]]).key_and_value,idx*2+1),charset_idx.id))
end
function M.check_charset_get_charset(charset)
    local cid=check_charset_get_id(charset)
    return vars.charset_table[cid]
end
local function cons_to_unsigned(obj)
    local val
    if lisp.fixnatp(obj) then
        val=lisp.fixnum(obj)
    elseif lisp.consp(obj) and lisp.fixnump(lisp.xcar(obj)) then
        local top=lisp.fixnum(lisp.xcar(obj))
        local rest=lisp.xcdr(obj)
        if lisp.consp(rest) then
            error('TODO')
        elseif overflow.min<=top and top<=overflow.max then
            if lisp.consp(rest) then
                error('TODO')
            end
            if lisp.fixnatp(rest) and lisp.fixnum(rest)<=0xffff then
                val=tonumber(bit.tohex(bit.bor(bit.lshift(top,16),lisp.fixnum(rest))),16)
            end
        else
            error('TODO')
        end
    else
        error('TODO')
    end
    if not val or val>0xffffffff then
        signal.error('Not an in-range integer, integral float, or cons of integers')
    end
    return val
end
F.define_charset_internal={'define-charset-internal',charset_arg.max,-2,0,[[For internal use only.
usage: (define-charset-internal ...)]]}
function F.define_charset_internal.f(args)
    if #args~=charset_arg.max then
        vars.F.signal(vars.Qwrong_number_of_arguments,vars.F.cons(lread.intern_c_string('define-charset-internal'),lisp.make_fixnum(#args)))
    end
    local hash_table=(vars.charset_hash_table --[[@as nelisp._hash_table]])
    local charset={}
    ---@cast charset nelisp.charset
    local attrs=alloc.make_vector(charset_idx.max,'nil')

    lisp.check_symbol(args[charset_arg.name])
    lisp.aset(attrs,charset_idx.name,args[charset_arg.name])

    local val=args[charset_arg.code_space]
    local dimension=0
    do
        local i=0
        local nchars=1
        charset.code_space={}
        while true do
            local min_byte_obj=vars.F.aref(val,lisp.make_fixnum(i*2))
            local max_byte_obj=vars.F.aref(val,lisp.make_fixnum(i*2+1))
            local min_byte=lisp.check_fixnum_range(min_byte_obj,0,255)
            local max_byte=lisp.check_fixnum_range(max_byte_obj,min_byte,255)
            charset.code_space[i*4]=min_byte
            charset.code_space[i*4+1]=max_byte
            charset.code_space[i*4+2]=max_byte-min_byte+1
            if max_byte>0 then
                dimension=dimension+1
            end
            if i==3 then
                break
            end
            nchars=nchars*charset.code_space[i*4+2]
            charset.code_space[i*4+3]=nchars
            i=i+1
        end
    end

    val=args[charset_arg.dimension]
    charset.dimension=not lisp.nilp(val) and lisp.check_fixnum_range(val,1,4) or dimension

    charset.code_linear_p=(
    charset.dimension==1
    or (charset.code_space[2]==256
    and (charset.dimension==2
    or (charset.code_space[6]==256
    and (charset.dimension==3
    or charset.code_space[10]==256)))))

    if not charset.code_linear_p then
        charset.code_space_mask={}
        for i=0,255 do
            charset.code_space_mask[i]=0
        end
        for i=0,3 do
            for j=charset.code_space[i*4],charset.code_space[i*4+1] do
                charset.code_space_mask[j]=bit.bor(charset.code_space_mask[j],bit.lshift(1,i))
            end
        end
    end

    charset.iso_chars_96=charset.code_space[2]==96

    charset.min_code=tonumber(bit.tohex(bit.bor(charset.code_space[0],
        bit.lshift(charset.code_space[4],8),
        bit.lshift(charset.code_space[8],16),
        bit.lshift(charset.code_space[12],24))),16)
    charset.max_code=tonumber(bit.tohex(bit.bor(charset.code_space[1],
        bit.lshift(charset.code_space[5],8),
        bit.lshift(charset.code_space[9],16),
        bit.lshift(charset.code_space[13],24))),16)
    charset.char_index_offset=0

    val=args[charset_arg.min_code]
    if not lisp.nilp(val) then
        local code=cons_to_unsigned(val)
        if (code<charset.min_code or code>charset.max_code) then
            signal.args_out_of_range(lisp.make_fixnum(charset.min_code),lisp.make_fixnum(charset.max_code),val)
        end
        charset.char_index_offset=code_point_to_index(charset,code)
        charset.min_code=code
    end

    val=args[charset_arg.max_code]
    if not lisp.nilp(val) then
        local code=cons_to_unsigned(val)
        if (code<charset.min_code or code>charset.max_code) then
            signal.args_out_of_range(lisp.make_fixnum(charset.min_code),lisp.make_fixnum(charset.max_code),val)
        end
        charset.max_code=code
    end

    charset.compact_codes_p=charset.max_code<0x10000

    val=args[charset_arg.invalid_code]
    if lisp.nilp(val) then
        if charset.min_code>0 then
            charset.invalid_code=0
        else
            if charset.max_code<=0xffffffff then
                charset.invalid_code=charset.max_code+1
            else
                signal.error('Attribute :invalid-code must be specified')
            end
        end
    else
        charset.invalid_code=cons_to_unsigned(val)
    end

    val=args[charset_arg.iso_final]
    if lisp.nilp(val) then
        charset.iso_final=-1
    else
        lisp.check_fixnum(val)
        if lisp.fixnum(val)<b'0' or lisp.fixnum(val)>127 then
            signal.error('Invalid iso-final-char: %d',lisp.fixnum(val))
        end
        charset.iso_final=lisp.fixnum(val)
    end

    val=args[charset_arg.iso_revision]
    charset.iso_revision=not lisp.nilp(val) and lisp.check_fixnum_range(val,-1,63) or -1

    val=args[charset_arg.emacs_mule_id]
    if lisp.nilp(val) then
        charset.emacs_mule_id=-1
    else
        lisp.fixnatp(val)
        if (lisp.fixnum(val)>0 and lisp.fixnum(val)<=128) or lisp.fixnum(val)>=256 then
            signal.error('Invalid emacs-mule-id: %d',lisp.fixnum(val))
        end
        charset.emacs_mule_id=lisp.fixnum(val)
    end

    charset.ascii_compatible_p=not lisp.nilp(args[charset_arg.ascii_compatible_p])

    charset.supplementary_p=not lisp.nilp(args[charset_arg.supplementary_p])

    charset.unified_p=false

    charset.fast_map={}
    for i=0,189 do
        charset.fast_map[i]=0
    end

    if not lisp.nilp(args[charset_arg.code_offset]) then
        val=args[charset_arg.code_offset]
        require'nelisp.chars'.check_character(val)

        charset.method=charset_method.offset
        charset.code_offset=lisp.fixnum(val)

        local i=code_point_to_index(charset,charset.max_code)
        if (b.MAX_CHAR-charset.code_offset)<i then
            signal.error('Unsupported max char: %d',charset.max_char)
            -- Hmm, `charset.max_char` is not yet set, is this a bug?
        end
        charset.max_char=i+charset.code_offset
        i=code_point_to_index(charset,charset.min_code)
        charset.min_char=i+charset.code_offset

        i=bit.lshift(bit.rshift(charset.min_char,7),7)
        while i<0x10000 and i<=charset.max_char do
            charset_fast_map_set(i,charset.fast_map)
            i=i+128
        end
        i=bit.lshift(bit.rshift(charset.min_char,12),12)
        while i<=charset.max_char do
            charset_fast_map_set(i,charset.fast_map)
            i=i+0x1000
        end
        if charset.code_offset==0 and charset.max_char>=0x80 then
            charset.ascii_compatible_p=true
        end
    elseif not lisp.nilp(args[charset_arg.map]) then
        val=args[charset_arg.map]
        lisp.aset(attrs,charset_idx.map,val)
        charset.method=charset_method.map
    elseif not lisp.nilp(args[charset_arg.subset]) then
        val=args[charset_arg.subset]
        local parent=vars.F.car(val)
        local parent_charset=M.check_charset_get_charset(parent)
        local parent_min_code=vars.F.nth(lisp.make_fixnum(1),val)
        lisp.check_fixnat(parent_min_code)
        local parent_max_code=vars.F.nth(lisp.make_fixnum(2),val)
        lisp.check_fixnat(parent_max_code)
        local parent_code_offset=vars.F.nth(lisp.make_fixnum(3),val)
        lisp.check_fixnum(parent_code_offset)
        lisp.aset(attrs,charset_idx.subset,
            vars.F.vector({lisp.make_fixnum(parent_charset.id),
                parent_min_code,parent_max_code,parent_code_offset}))
        charset.method=charset_method.subset
        for i=0,#parent_charset.fast_map-1 do
            charset.fast_map[i]=parent_charset.fast_map[i]
        end
        charset.min_char=parent_charset.min_char
        charset.max_char=parent_charset.max_char
    elseif not lisp.nilp(args[charset_arg.superset]) then
        val=args[charset_arg.superset]
        charset.method=charset_method.superset
        val=vars.F.copy_sequence(val)
        charset.min_char=b.MAX_CHAR
        charset.max_char=0
        while not lisp.nilp(val) do
            local this_id,offset
            local elt=vars.F.car(val)
            if lisp.consp(elt) then
                local car_part=lisp.xcar(elt)
                local cdr_part=lisp.xcdr(elt)
                this_id=check_charset_get_id(car_part)
                offset=lisp.check_fixnum_range(cdr_part,overflow.min,overflow.max)
            else
                this_id=check_charset_get_id(elt)
                offset=0
            end
            lisp.xsetcar(val,vars.F.cons(lisp.make_fixnum(this_id),lisp.make_fixnum(offset)))
            local this_charset=assert(vars.charset_table[this_id])
            if charset.min_char>this_charset.min_char then
                charset.min_char=this_charset.min_char
            end
            if charset.max_char<this_charset.max_char then
                charset.max_char=this_charset.max_char
            end
            for i=0,189 do
                charset.fast_map[i]=bit.bor(charset.fast_map[i],this_charset.fast_map[i])
            end
            val=vars.F.cdr(val)
        end
    else
        signal.error('None of :code-offset, :map, :parents are specified')
    end

    val=args[charset_arg.unify_map]
    if not lisp.nilp(val) and not lisp.stringp(val) then
        lisp.check_vector(val)
    end
    lisp.aset(attrs,charset_idx.unify_map,val)

    lisp.check_list(args[charset_arg.plist])
    lisp.aset(attrs,charset_idx.plist,args[charset_arg.plist])

    local hash_code
    charset.hash_index,hash_code=fns.hash_lookup(hash_table,args[charset_arg.name])

    local id,new_definition_p
    if charset.hash_index>=0 then
        error('TODO')
    else
        charset.hash_index=fns.hash_put(hash_table,args[charset_arg.name],attrs,hash_code)
        id=#vars.charset_table
        new_definition_p=true
    end

    lisp.aset(attrs,charset_idx.id,lisp.make_fixnum(id))
    charset.id=id
    vars.charset_table[id]=charset

    if charset.method==charset_method.map then
        load_charset(charset,0)
        vars.charset_table[id]=charset
    end

    if charset.iso_final>=0 then
        vars.iso_charset_table[charset.dimension-1][charset.iso_chars_96 and 1 or 0][charset.iso_final]=id
        if new_definition_p then
            vars.iso_2022_charset_list=vars.F.nconc({vars.iso_2022_charset_list,lisp.list(lisp.make_fixnum(id))})
        end
        if vars.iso_charset_table[1][0][b'J']==id then
            vars.charset_jisx0201_roman=id
        elseif vars.iso_charset_table[2][0][b'@']==id then
            vars.charset_jisx0208_1978=id
        elseif vars.iso_charset_table[2][0][b'B']==id then
            vars.charset_jisx0208=id
        elseif vars.iso_charset_table[2][0][b'C']==id then
            vars.charset_ksc5601=id
        end
    end

    if charset.emacs_mule_id>=0 then
        vars.emacs_mule_charset[charset.emacs_mule_id]=id
        if charset.emacs_mule_id<0xa0 then
            vars.emacs_mule_bytes[charset.emacs_mule_id]=charset.dimension+1
        else
            vars.emacs_mule_bytes[charset.emacs_mule_id]=charset.dimension+2
        end
        if new_definition_p then
            vars.emacs_mule_charset_list=vars.F.nconc({vars.emacs_mule_charset_list,lisp.list(lisp.make_fixnum(id))})
        end
    end

    if new_definition_p then
        vars.V.charset_list=vars.F.cons(args[charset_arg.name],vars.V.charset_list)
        if charset.supplementary_p then
            vars.charset_ordered_list=vars.F.nconc({vars.charset_ordered_list,lisp.list(lisp.make_fixnum(id))})
        else
            local tail=vars.charset_ordered_list
            while lisp.consp(tail) do
                local cs=vars.charset_table[lisp.fixnum(lisp.xcar(tail))]
                if cs.supplementary_p then break end
                tail=lisp.xcdr(tail)
            end
            if lisp.eq(tail,vars.charset_ordered_list) then
                vars.charset_ordered_list=vars.F.cons(lisp.make_fixnum(id),vars.charset_ordered_list)
            elseif lisp.nilp(tail) then
                vars.charset_ordered_list=vars.F.nconc({vars.charset_ordered_list,lisp.list(lisp.make_fixnum(id))})
            else
                error('TODO')
            end
        end
        vars.charset_ordered_list_tick=vars.charset_ordered_list_tick+1
    end

    return vars.Qnil
end
local function check_charset_get_attr(x)
    if not lisp.symbolp(x) then
        signal.wrong_type_argument(vars.Qcharsetp,x)
    end
    local attr=charset_symbol_attributes(x)
    if lisp.nilp(attr) then
        signal.wrong_type_argument(vars.Qcharsetp,x)
    end
    return attr
end
F.set_charset_plist={'set-charset-plist',2,2,0,[[Set CHARSET's property list to PLIST.]]}
function F.set_charset_plist.f(charset,plist)
    local attrs=check_charset_get_attr(charset)
    lisp.aset(attrs,charset_idx.plist,plist)
    return plist
end
F.charset_plist={'charset-plist',1,1,0,[[Return the property list of CHARSET.]]}
function F.charset_plist.f(charset)
    local attr=check_charset_get_attr(charset)
    return lisp.aref(attr,charset_idx.plist)
end
F.define_charset_alias={'define-charset-alias',2,2,0,[[Define ALIAS as an alias for charset CHARSET.]]}
function F.define_charset_alias.f(alias,charset)
    local attrs=check_charset_get_attr(charset)
    vars.F.puthash(alias,attrs,vars.charset_hash_table)
    vars.V.charset_list=vars.F.cons(alias,vars.V.charset_list)
    return vars.Qnil
end
F.unify_charset={'unify-charset',1,3,0,[[Unify characters of CHARSET with Unicode.
This means reading the relevant file and installing the table defined
by CHARSET's `:unify-map' property.

Optional second arg UNIFY-MAP is a file name string or a vector.  It has
the same meaning as the `:unify-map' attribute in the function
`define-charset' (which see).

Optional third argument DEUNIFY, if non-nil, means to de-unify CHARSET.]]}
function F.unify_charset.f(charset,unify_map,deunify)
    local id=check_charset_get_id(charset)
    local cs=vars.charset_table[id]
    local attrs=charset_attributes(cs)
    if not lisp.nilp(deunify) and not cs.unified_p then
        return vars.Qnil
    elseif lisp.nilp(deunify) and cs.unified_p and not lisp.nilp(lisp.aref(attrs,charset_idx.deunifier)) then
        return vars.Qnil
    end
    cs.unified_p=false
    if lisp.nilp(deunify) then
        if cs.method~=charset_method.offset or cs.code_offset<0x110000 then
            signal.error("Can't unify charset: %s",lisp.sdata(lisp.symbol_name(charset)))
        end
        if lisp.nilp(unify_map) then
            unify_map=lisp.aref(attrs,charset_idx.unify_map)
        else
            error('TODO')
        end
        if lisp.nilp(vars.char_unify_table) then
            vars.char_unify_table=vars.F.make_char_table(vars.Qnil,vars.Qnil)
        end
        chartab.set_range(vars.char_unify_table,cs.min_code,cs.max_code,charset)
        cs.unified_p=true
    else
        error('TODO')
    end
    return vars.Qnil
end
local function charsetp(obj)
    return fns.hash_lookup((vars.charset_hash_table --[[@as nelisp._hash_table]]),obj)>=0
end
F.charsetp={'charsetp',1,1,0,[[Return non-nil if and only if OBJECT is a charset.]]}
function F.charsetp.f(obj)
    return charsetp(obj) and vars.Qt or vars.Qnil
end
function M.init()
    vars.charset_hash_table=vars.F.make_hash_table({vars.QCtest,vars.Qeq})
    vars.charset_table={}
    vars.iso_charset_table={}
    vars.emacs_mule_charset={}
    vars.emacs_mule_bytes={}
    vars.charset_ordered_list_tick=0
    for i=0,255 do
        vars.emacs_mule_charset[i]=-1
        vars.emacs_mule_bytes[i]=1
    end
    vars.emacs_mule_bytes[0x9a]=3
    vars.emacs_mule_bytes[0x9b]=3
    vars.emacs_mule_bytes[0x9c]=4
    vars.emacs_mule_bytes[0x9d]=4
    vars.iso_2022_charset_list=vars.Qnil
    vars.emacs_mule_charset_list=vars.Qnil
    vars.charset_ordered_list=vars.Qnil
    vars.char_unify_table=vars.Qnil
    vars.charset_jisx0201_roman=-1
    vars.charset_jisx0208_1978=-1
    vars.charset_jisx0208=-1
    vars.charset_ksc5601=-1
    for i=0,2 do
        vars.iso_charset_table[i]={[0]={},[1]={}}
    end

    vars.charset_ascii=define_charset_internal(vars.Qascii,1,'\x00\x7F\0\0\0\0\0',0,127,b'B',-1,0,true,false,0)
    vars.charset_iso_8859_1=define_charset_internal(vars.Qiso_8859_1,1,'\x00\xFF\0\0\0\0\0',0,255,-1,-1,-1,true,false,0)
    vars.charset_unicode=define_charset_internal(vars.Qunicode,3,'\x00\xFF\x00\xFF\x00\x10\0',0,b.MAX_UNICODE_CHAR,-1,0,-1,true,false,0)
    vars.charset_emacs=define_charset_internal(vars.Qemacs,3,'\x00\xFF\x00\xFF\x00\x3F\0',0,b.MAX_5_BYTE_CHAR,-1,0,-1,true,true,0)
    vars.charset_eight_bit=define_charset_internal(vars.Qeight_bit,1,'\x80\xFF\0\0\0\0\0',128,255,-1,0,-1,false,true,b.MAX_5_BYTE_CHAR+1)
end
function M.init_syms()
    vars.defsubr(F,'set_charset_plist')
    vars.defsubr(F,'charset_plist')
    vars.defsubr(F,'define_charset_internal')
    vars.defsubr(F,'define_charset_alias')
    vars.defsubr(F,'unify_charset')
    vars.defsubr(F,'charsetp')

    vars.defsym('Qemacs','emacs')
    vars.defsym('Qiso_8859_1','iso-8859-1')
    vars.defsym('Qeight_bit','eight-bit')
    vars.defsym('Qunicode','unicode')
    vars.defsym('Qascii','ascii')
    vars.defsym('QCname',':name')
    vars.defsym('QCascii_compatible_p',':ascii-compatible-p')

    vars.defvar_lisp('charset_list','charset-list',[[List of all charsets ever defined.]])
    vars.V.charset_list=vars.Qnil
end
return M
