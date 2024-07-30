
/* 64 Bytes : security monitor's signature by device */
PROVIDE( sanctum_CDI = . );
. += 0x40;

/* 32 Bytes : security monitor's signature by device */
PROVIDE( sanctum_ECA_pk = . );
. += 0x40;

/* 64 Bytes : security monitor's signature by device */
PROVIDE( sanctum_sm_hash_to_check = . );
. += 0x40;

/* 64 Bytes : security monitor's signature by device */
PROVIDE( sanctum_sm_key_pub = . );
. += 0x40;

/* 64 Bytes : security monitor's signature by device */
PROVIDE( test = . );
. += 0x40;

/* 64 Bytes : security monitor's signature by device */
PROVIDE( sanctum_sm_signature_drk = . );
. += 0x40;

/* 64 Bytes : security monitor's signature by device */
PROVIDE( sanctum_device_root_key_pub = . );
. += 0x40;


/* 4 Byte : security monitor's signature by device */
PROVIDE( sanctum_length_cert = . );
. += 0x04;

/* 256 Bytes : security monitor's signature by device */
PROVIDE( sanctum_cert_sm = . );
. += 0x100;





