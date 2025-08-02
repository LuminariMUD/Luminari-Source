
#define DEITY_NONE			0

#if defined(CAMPAIGN_FR)
// FORGOTTEN REALMS DEITIES

// Faerunian Pantheon

#define DEITY_AKADI 			1
#define DEITY_AURIL			2
#define DEITY_AZUTH			3
#define DEITY_BANE			4
#define DEITY_BESHABA			5
#define DEITY_CHAUNTEA			6
#define DEITY_CYRIC			7
#define DEITY_DENEIR			8
#define DEITY_ELDATH			9
#define DEITY_FINDER_WYVERNSPUR 	10
#define DEITY_GARAGOS			11
#define DEITY_GARGAUTH			12
#define DEITY_GOND			13
#define DEITY_GRUMBAR			14
#define DEITY_GWAERON_WINDSTROM		15
#define DEITY_HELM			16
#define DEITY_HOAR			17
#define DEITY_ILMATER			18
#define DEITY_ISTISHIA			19
#define DEITY_JERGAL			20
#define DEITY_KELEMVOR			21
#define DEITY_KOSSUTH			22
#define DEITY_LATHANDER			23
#define DEITY_LLIIRA			24
#define DEITY_LOVIATAR			25
#define DEITY_LURUE			26
#define DEITY_MALAR			27
#define DEITY_MASK			28
#define DEITY_MIELIKKI			29
#define DEITY_MILIL			30
#define DEITY_MYSTRA			31
#define DEITY_NOBANION			32
#define DEITY_OGHMA			33
#define DEITY_RED_KNIGHT		34
#define DEITY_SAVRAS			35
#define DEITY_SELUNE			36
#define DEITY_SHAR			37
#define DEITY_SHARESS			38
#define DEITY_SHAUNDAKUL		39
#define DEITY_SHIALLIA			40
#define DEITY_SIAMORPHE			41
#define DEITY_SILVANUS			42
#define DEITY_SUNE			43
#define DEITY_TALONA			44
#define DEITY_TALOS			45
#define DEITY_TEMPUS			46
#define DEITY_TIAMAT			47
#define DEITY_TORM			48
#define DEITY_TYMORA			49
#define DEITY_TYR			50
#define DEITY_UBTAO			51
#define DEITY_ULUTIU			52
#define DEITY_UMBERLEE			53
#define DEITY_UTHGAR			54
#define DEITY_VALKUR			55
#define DEITY_VELSHAROON		56
#define DEITY_WAUKEEN			57

// Mulhorandi Pantheon

#define DEITY_ANHUR			58
#define DEITY_GEB			59
#define DEITY_HATHOR			60
#define DEITY_HORUS_RE			61
#define DEITY_ISIS			62
#define DEITY_NEPHTHYS			63
#define DEITY_OSIRIS			64
#define DEITY_SEBEK			65
#define DEITY_SET			66
#define DEITY_THOTH			67

// Drow Pantheon

#define DEITY_EILISTRAEE		68
#define DEITY_GHAUNADAUR		69
#define DEITY_KIARANSALEE		70
#define DEITY_LOLTH			71
#define DEITY_SELVETARM			72
#define DEITY_VHAERAUN			73

// Dwarven Pantheon

#define DEITY_ABBATHOR			74
#define DEITY_BERRONAR_TRUESILVER	75
#define DEITY_CLANGEDDIN_SILVERBEARD	76
#define DEITY_DEEP_DUERRA		77
#define DEITY_DUGMAREN_BRIGHTMANTLE	78
#define DEITY_DUMATHOIN			79
#define DEITY_GORM_GULTHYN		80
#define DEITY_HAELA_BRIGHTAXE		81
#define DEITY_LADUGUER			82
#define DEITY_MARTHAMMOR_DUIN		83
#define DEITY_MORADIN			84
#define DEITY_SHARINDLAR		85
#define DEITY_THARD_HARR		86
#define DEITY_VERGADAIN			87

// Elven Pantheon

#define DEITY_AERDRIE_FAENYA		88
#define DEITY_ANGHARRADH		89
#define DEITY_CORELLON_LARETHIAN	90
#define DEITY_DEEP_SASHELAS		91
#define DEITY_EREVAN_ILESERE		92
#define DEITY_FENMAREL_MESTARINE	93
#define DEITY_HANALI_CELANIL		94
#define DEITY_LABELAS_ENORETH		95
#define DEITY_RILLIFANE_RALLATHIL	96
#define DEITY_SEHANINE_MOONBOW		97
#define DEITY_SHEVARASH			98
#define DEITY_SOLONOR_THELANDIRA	99

// Gnome Pantheon

#define DEITY_BAERVAN_WILDWANDERER	100
#define DEITY_BARAVAR_CLOAKSHADOW	101
#define DEITY_CALLARDURAN_SMOOTHHANDS	102
#define DEITY_FLANDAL_STEELSKIN		103
#define DEITY_GAERDAL_IRONHAND		104
#define DEITY_GARL_GLITTERGOLD		105
#define DEITY_SEGOJAN_EARTHCALLER	106
#define DEITY_URDLEN			107

// Halfling Pantheon

#define DEITY_ARVOREEN			108
#define DEITY_BRANDOBARIS		109
#define DEITY_CYRROLLALEE		110
#define DEITY_SHEELA_PERYROYL		111
#define DEITY_UROGALAN			112
#define DEITY_YONDALLA			113

// Orc Pantheon

#define DEITY_BAHGTRU			114
#define DEITY_GRUUMSH			115
#define DEITY_ILNEVAL			116
#define DEITY_LUTHIC			117
#define DEITY_SHARGAAS			118
#define DEITY_YURTRUS			119


#define NUM_DEITIES			120 // One more than the last entry please :)

#define DEITY_PANTHEON_NONE		0
#define DEITY_PANTHEON_ALL		1
#define DEITY_PANTHEON_FAERUNIAN	2
#define DEITY_PANTHEON_FR_DWARVEN	3
#define DEITY_PANTHEON_FR_DROW	        4
#define DEITY_PANTHEON_FR_ELVEN	        5
#define DEITY_PANTHEON_FR_GNOME	        6
#define DEITY_PANTHEON_FR_HALFLING      7
#define DEITY_PANTHEON_FR_ORC 	        8

#define NUM_PANTHEONS 9

#define DEITY_LIST_ALL 1
#define DEITY_LIST_GOOD 2
#define DEITY_LIST_NEUTRAL 3
#define DEITY_LIST_EVIL 4
#define DEITY_LIST_LAWFUL 5
#define DEITY_LIST_CHAOTIC 6
#define DEITY_LIST_FAERUN 7
#define DEITY_LIST_DWARVEN 8
#define DEITY_LIST_DROW 9
#define DEITY_LIST_ELVEN 10
#define DEITY_LIST_GNOME 11
#define DEITY_LIST_HALFLING 12
#define DEITY_LIST_ORC 13

#elif defined(CAMPAIGN_DL)

#define DEITY_PALADINE 1
#define DEITY_MISHAKAL 2
#define DEITY_KIRI_JOLITH 3
#define DEITY_HABBAKUK 4
#define DEITY_MAJERE 5
#define DEITY_BRANCHALA 6
#define DEITY_SOLINARI 7

#define DEITY_GILEAN 8
#define DEITY_REORX 9
#define DEITY_ZIVILYN 10
#define DEITY_CHISLEV 11
#define DEITY_SIRRION 12
#define DEITY_SHINARE 13
#define DEITY_LUNITARI 14

#define DEITY_TAKHISIS 15
#define DEITY_MORGION 16
#define DEITY_CHEMOSH 17
#define DEITY_SARGONNAS 18
#define DEITY_ZEBOIM 19
#define DEITY_HIDDUKEL 20 
#define DEITY_NUITARI 21

#define NUM_DEITIES 22

#define DEITY_PANTHEON_NONE		0
#define DEITY_PANTHEON_ALL		1
#define NUM_PANTHEONS 2

#define DEITY_LIST_ALL 1
#define DEITY_LIST_GOOD 2
#define DEITY_LIST_NEUTRAL 3
#define DEITY_LIST_EVIL 4
#define DEITY_LIST_LAWFUL 5
#define DEITY_LIST_CHAOTIC 6

#else

#define DEITY_PANTHEON_NONE		0
#define DEITY_PANTHEON_ALL		1
#define NUM_PANTHEONS 2

#define DEITY_LIST_ALL 1
#define DEITY_LIST_GOOD 2
#define DEITY_LIST_NEUTRAL 3
#define DEITY_LIST_EVIL 4
#define DEITY_LIST_LAWFUL 5
#define DEITY_LIST_CHAOTIC 6

    #define NUM_DEITIES 1
#endif

struct deity_info
{

    const char *name;
    int ethos;
    int alignment;
    ubyte domains[6];
    ubyte favored_weapon;
    sbyte pantheon;
    const char *portfolio;
    const char *description;
    const char *alias;
    const char *symbol;
    const char *worshipper_alignments;
    const char *follower_names;
    bool new_deity_system;
};

extern struct deity_info deity_list[NUM_DEITIES];
