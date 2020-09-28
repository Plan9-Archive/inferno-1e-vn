typedef struct{char *name; long sig; void (*fn)(void*); int size; int np; uchar map[16];} Runtab;
Runtab Keyringmodtab[]={
	"IPint.add",0xa47c1b24,IPint_add,40,2,{0x0,0xc0,},
	"auth",0x9c576bb,Keyring_auth,48,2,{0x0,0xc0,},
	"IPint.b64toip",0xa803ee03,IPint_b64toip,40,2,{0x0,0x80,},
	"IPint.bits",0xeb4c9bad,IPint_bits,40,2,{0x0,0x80,},
	"certtostr",0xbc65254a,Keyring_certtostr,40,2,{0x0,0x80,},
	"IPint.cmp",0x79774f9e,IPint_cmp,40,2,{0x0,0xc0,},
	"descbc",0xac616ba,Keyring_descbc,48,2,{0x0,0xc0,},
	"desecb",0xac616ba,Keyring_desecb,48,2,{0x0,0xc0,},
	"dessetup",0x44452583,Keyring_dessetup,40,2,{0x0,0xc0,},
	"dhparams",0x6abb2418,Keyring_dhparams,40,0,{0},
	"IPint.div",0x4672bf61,IPint_div,40,2,{0x0,0xc0,},
	"IPint.eq",0x79774f9e,IPint_eq,40,2,{0x0,0xc0,},
	"IPint.expmod",0xe6105024,IPint_expmod,48,2,{0x0,0xe0,},
	"genSK",0xadd8cbd9,Keyring_genSK,48,2,{0x0,0xc0,},
	"genSKfromPK",0x5416d1ee,Keyring_genSKfromPK,40,2,{0x0,0xc0,},
	"getbytearray",0x4e02ce80,Keyring_getbytearray,40,2,{0x0,0x80,},
	"getmsg",0xd9de1bb7,Keyring_getmsg,40,2,{0x0,0x80,},
	"getstring",0x92f10e56,Keyring_getstring,40,2,{0x0,0x80,},
	"IPint.inttoip",0x95dc8b6d,IPint_inttoip,40,0,{0},
	"IPint.iptob64",0xfab4eb8a,IPint_iptob64,40,2,{0x0,0x80,},
	"IPint.iptobytes",0xc8e5162d,IPint_iptobytes,40,2,{0x0,0x80,},
	"IPint.iptoint",0xeb4c9bad,IPint_iptoint,40,2,{0x0,0x80,},
	"md5",0x7656377,Keyring_md5,48,2,{0x0,0xb0,},
	"IPint.mul",0xa47c1b24,IPint_mul,40,2,{0x0,0xc0,},
	"IPint.neg",0x491fbd11,IPint_neg,40,2,{0x0,0x80,},
	"pktostr",0xfb4e61ba,Keyring_pktostr,40,2,{0x0,0x80,},
	"putbytearray",0x7cfef557,Keyring_putbytearray,48,2,{0x0,0xc0,},
	"puterror",0xd2526222,Keyring_puterror,40,2,{0x0,0xc0,},
	"putstring",0xd2526222,Keyring_putstring,40,2,{0x0,0xc0,},
	"IPint.random",0x70bcd6f1,IPint_random,40,0,{0},
	"readauthinfo",0xb2c82015,Keyring_readauthinfo,40,2,{0x0,0x80,},
	"sendmsg",0x7cfef557,Keyring_sendmsg,48,2,{0x0,0xc0,},
	"sha",0x7656377,Keyring_sha,48,2,{0x0,0xb0,},
	"sign",0xdacb7a7e,Keyring_sign,48,2,{0x0,0xb0,},
	"sktopk",0x6f74c7c9,Keyring_sktopk,40,2,{0x0,0x80,},
	"sktostr",0xfb4e61ba,Keyring_sktostr,40,2,{0x0,0x80,},
	"strtocert",0x2c0ee68a,Keyring_strtocert,40,2,{0x0,0x80,},
	"strtopk",0xcc511522,Keyring_strtopk,40,2,{0x0,0x80,},
	"strtosk",0xcc511522,Keyring_strtosk,40,2,{0x0,0x80,},
	"IPint.sub",0xa47c1b24,IPint_sub,40,2,{0x0,0xc0,},
	"verify",0x8b5b9f76,Keyring_verify,48,2,{0x0,0xe0,},
	"writeauthinfo",0x5ba03002,Keyring_writeauthinfo,40,2,{0x0,0xc0,},
	0
};