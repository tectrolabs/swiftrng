/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 Unit tests for the SHA-256 and SHA-512 code inside SwiftRngApi, which post-processes the random bytes read
 from the device. The expected digests were generated independently with Python's hashlib. The API hashes
 whole 32 bit (SHA-256) or 64 bit (SHA-512) words, which are taken as big-endian bytes of the message.
 */

/**
 *    @file test_sha.cpp
 *
 *    @brief Known answer tests for the SHA-256 and SHA-512 implementations in SwiftRngApi.
 */
#include <cstdio>
#include "private_access.h"
#include "test_framework.h"

using swiftrng::SwiftRngApi;

struct Sha256Vector {
	int words;
	const char *digest;
};

struct Sha512Vector {
	int words;
	const char *digest;
};

// Message of n words, word i = (i * 2654435761 + 12345) mod 2^32
static const Sha256Vector c_sha256_vectors[] = {
	{1, "6b12f6f5b9234b3206c99bf2c25dc463da1fa11327f17215df2a708e10a557ca"},
	{2, "bce525c8cab5cd8597e36db6bdfb66cd32ec23d54d4e20efd3106136f7df5cf8"},
	{3, "b7daf5e7e4046ea95dbbfba594f00db0713d9041b873c6c4a23a7bcdf012a7a0"},
	{4, "e16f9189b5e28585aaa3c88ac30b39f3c227dacd1f96002055d4daf0bb801222"},
	{5, "a74761f14959076814a8adb9b84e4bc673fd9f1ccfdf4c85e8e5aa370ae2341d"},
	{6, "e424aa9c1257e29ec2076dcbcb2da6ded00491dedb7e9d6f72fb940bbae1caa2"},
	{7, "6cc01034a4dff3a8c638288fa4fc22abaa683da11ce30ca18ffcca73bdbe6f10"},
	{8, "c799f7312d663b32ae31b9e83c7a17ab5e7034b79daaef4c9bc90042fcdd3647"},
	{9, "ef32a23c2545f40c4c60f11e0b72b2d0f49a52fd5f01ee2016458f37ae386d1b"},
	{10, "d00bc37be9f76329fdbe7216bbff1483cc7eb686a0855d5a5290a150bcfdc1ed"},
	{11, "494aa4c514ad4f2dedaccc99014a7edcccaabfb72c19b95b757e597954fb31a6"},
	{12, "18ef1f0daa264cd20e4e4b0ed6cf3116f6a850fe0dce687bb8ce47ba8ded9673"},
	{13, "78312da9aee97a223fdd3b14b385264d3b86a9f2b4df33856f370a1a0baa1c21"},
	{14, "168f6a25312d539026549ec7143f53cd55233cc5dc30d6888ca2b007fd0eb747"},
	{15, "46cb6b8ef329fa2e39825995dcba73f533529e4984e073a42e8ef890a53c7a33"},
	{16, "74e17114c13bfd8b7a2170fe72c707460a3e7f55c0f37d1046aeb9f970f4cc3a"},
	{17, "7a07d9556df61a0bb05480fddfea294153658cfb91ab68312988640bdee14d19"},
	{18, "c6a29228f8d4573b7ba8ea951c0105ed024c87d234d1497517046125ea3707cd"},
	{19, "8e30d4765969c1d6d5abbb8a03e24e469c02eac16098eaaa4775ca36d7f26d3c"},
	{20, "6cf017e0d8c495d16166c8ac5880a6321aca77287fb8b4515b8dbc873f658436"},
	{21, "baf4dc7cd9c4e86f6db47d92ef46f181607f165e5ccd877b47884df28a0951be"},
	{22, "90ed432000d0a38101fd5d50382eee2b6edbef716d6c1d7cb8189c5119441ad6"},
	{23, "061b9f01e32248d523b14ce2c74f126c8c22c6d481054a71dc2c32d960509cca"},
	{24, "abb658b97e61c093a3769e5fea669bc7728f1ddde2cc05ad0524c66e7fa7c158"},
	{25, "577cec90ee4234812173b35b0a9cf9499aed522e240383ec4c8e1b5f86ad8b4e"},
	{26, "90a3ad4320358cb7bbf81cc57164c6e56d7d3e7fe1dfbd456064f3e3fcd8a657"},
	{27, "ae8a55b7f3931a2bfd870df9f05de9516807d9183b1e134c1f1520c47cfad2bb"},
	{28, "f31018fa2b4166c0a12325676fbc6e32390251f560b7e04e8a64ccf40fa163f7"},
	{29, "92222f5fb0520c8a9ec04e196931b8ff58df6394e6026cefbcae407b3e80e06c"},
	{30, "73f66755a1c1dae7cd855e13fd1837eeb96464d1a626fdd4239de85e0ba6745c"},
	{31, "bbcaa3082a7ade15bd5a67a7011062f407eb04edceef2ef696705000c84d52c9"},
	{32, "5ebd76c34394252ad02df40ec45b3c3fe76f53565c049c3071639817d815d844"},
	{33, "4a3faab29b99ad8c718a6fd5415f6bf7876e9267a1f558dd0c1f3e3811cc5f81"},
	{34, "f469167e7420c7bf02b93120983afa084452815aaddfb1b0ca2d8e50d26692cf"},
	{35, "825790c79af18eab4eea2049e2672be94f0d2bda86f3c3e928eaf6508dd9f8c2"},
	{36, "e457c69b9b45444166b793a64a3b286643c5e40a847be03b0d87366469b14f45"},
	{37, "ac2a7e01bee7a492be1a30a38f630de741dc8cffdf4929d7d7fdf8ebdcedd3ca"},
	{38, "da9b816f4c3a96bf91e75df65d6b134a7b937ea4b63172d7c746076e510e0d6e"},
	{39, "d291c887316e995929ea2d96a70c8419cf77beea81e87663d5256b48c6b11e4d"},
	{40, "2614c4d8668c9055584cb624c9296fbd29936d412555687275f6f5f2319ad056"},
};

// Message of n words, word i = (i * 0x9E3779B97F4A7C15 + 0x0123456789ABCDEF) mod 2^64
static const Sha512Vector c_sha512_vectors[] = {
	{1,
		"650161856da7d9f818e6047cf6b2092bc7aa3767d3495cfbefe2b710ed684a43"
		"ba933ea8286ef67d975e64e0482e5ebe0701788989396545b6badb3b0a136f19"},
	{2,
		"3835b1e298a5cd800da6945d148066f7f7c4ab2b43e020ee33cc74b3a009630f"
		"ffbf081befa63c69024fc875055f088bbdcf57853f6b508df956591495286acf"},
	{3,
		"e478e5f4ae0f736ac5178c2760d63e6c216272eae76a5a07b0df2a0895d926c8"
		"74793741b9328d56a9d132628d106ff2a5bafc3e6fd210370fe2031a6c5a0aa5"},
	{4,
		"ec66ac0a2067099e8d3c83567fd061a0b99bcf66d7851c246baca22094dcfab9"
		"8b9c20e7a401f02f19002d7f4da52a1a4e5b5cc0a998a0f8d09f93f2f18d27d5"},
	{5,
		"cbfe6488ffc830683e3d11aefcbcdeed4cf6ce767d1a11ae2436b1715373972d"
		"1bd829ed1be9c97bdaebf0f4801a8839e7a8f7f0d676ae642dc22620f48017cd"},
	{6,
		"caf2be6441ccddbeb0f3aad73b478bcbeeac2a7a8d409c143b1ff370f5079252"
		"5e4a4378877688a19fe4621bfd74e43ccafe78323f517629bcdf5d7bc4978ae7"},
	{7,
		"49d486d978f193e30f4c2fd522d8735d7d6f313c13d98a76b72f52bb603eb7ad"
		"ef0ba1c63ba844002b5004af4c1a4edfaa6842b4ebaa238350837ce85bd27f66"},
	{8,
		"7b2880848055b3b8cea56d26952f3199607014dccdc580396cdcaa2bf6aafe69"
		"368b6cb9bf80df365851fad9166cbe844196bf13bcf2f02288a8947e316334aa"},
	{9,
		"76b24175269934f27d6f67ac5a2818dd8bf40e2e574e95571681c401bce02573"
		"3e9e36eb56c6e3a91892de7056d80ea501c4ae7f92a8a305a6a2606a26ed7681"},
	{10,
		"ae795eafeaec141fc9a9fb54b38933aabfa8c92318d97f5d2e3eaaa2c5aa0e14"
		"30286e73eb801d10e99de68f849c33e0fed08ad55895b1c06b3a13265a080d62"},
	{11,
		"a0eab8a60d6aae8885a8bdca325b8a2cf728cbf877be1ab42e96483b5fda2c45"
		"167d5f71028b6aced8e074a19cb33766337f5c145eefe4a6da81d62940acf783"},
	{12,
		"6c1297f7bda0c29b8d495354162dd22007fe3b34104945888e0ac4eda7b6ec2b"
		"629a54703de10b6f6634a15a080b25bc1f0acab424ebca9b6a50ea48dfebcfda"},
	{13,
		"3cca40534c8b58f556fc325c13df71974f3f8fabb2d069840ab6fe2d66c86e32"
		"3ed3f300a5c4cc8649085f5f394aff26aec71c391857bd8763c0c6109c7f2238"},
	{14,
		"fff608d45421a6c610e10cae667d429338a70a594aa78dbbc866fc7d2467ddad"
		"1c462a5e9aba1506c0eaf4079818143bc486e889bf5b26faa16d50fc2ddf7a09"},
};

static uint32_t sha256_word(int i) {
	return (uint32_t)((uint64_t)i * 2654435761ULL + 12345ULL);
}

static uint64_t sha512_word(int i) {
	return (uint64_t)i * UINT64_C(0x9E3779B97F4A7C15) + UINT64_C(0x0123456789ABCDEF);
}

static std::string hex_words(const uint32_t *words, int count) {
	std::string out;
	char buf[16];
	for (int i = 0; i < count; ++i) {
		std::snprintf(buf, sizeof(buf), "%08x", words[i]);
		out += buf;
	}
	return out;
}

static std::string hex_words(const uint64_t *words, int count) {
	std::string out;
	char buf[24];
	for (int i = 0; i < count; ++i) {
		std::snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)words[i]);
		out += buf;
	}
	return out;
}

TEST(sha_api_object_is_initialized) {
	SwiftRngApi api;
	CHECK(api.is_context_initialized());
}

TEST(sha256_matches_known_answers_for_1_to_40_words) {
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	for (const Sha256Vector &vector : c_sha256_vectors) {
		std::vector<uint32_t> message((size_t)vector.words);
		for (int i = 0; i < vector.words; ++i) {
			message[(size_t)i] = sha256_word(i);
		}
		uint32_t digest[8] = {0};
		CHECK_EQ(api.sha256_generateHash(message.data(), (int16_t)vector.words, digest), 0);
		CHECK_EQ(hex_words(digest, 8), std::string(vector.digest));
	}
}

TEST(sha256_rejects_an_empty_message) {
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	uint32_t word = 1;
	uint32_t digest[8] = {0};
	CHECK_EQ(api.sha256_generateHash(&word, 0, digest), -1);
	CHECK_EQ(api.sha256_generateHash(&word, -3, digest), -1);
}

TEST(sha256_is_repeatable_and_does_not_depend_on_earlier_calls) {
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	uint32_t message[20];
	for (int i = 0; i < 20; ++i) {
		message[i] = sha256_word(i);
	}
	uint32_t first[8] = {0};
	uint32_t other[8] = {0};
	uint32_t second[8] = {0};
	REQUIRE(api.sha256_generateHash(message, 20, first) == 0);
	REQUIRE(api.sha256_generateHash(message, 5, other) == 0);
	REQUIRE(api.sha256_generateHash(message, 20, second) == 0);
	CHECK_EQ(hex_words(first, 8), hex_words(second, 8));
	CHECK(hex_words(first, 8) != hex_words(other, 8));
}

TEST(sha256_changing_one_bit_changes_the_digest) {
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	uint32_t message[8];
	for (int i = 0; i < 8; ++i) {
		message[i] = sha256_word(i);
	}
	uint32_t before[8] = {0};
	uint32_t after[8] = {0};
	REQUIRE(api.sha256_generateHash(message, 8, before) == 0);
	message[3] ^= 1;
	REQUIRE(api.sha256_generateHash(message, 8, after) == 0);
	CHECK(hex_words(before, 8) != hex_words(after, 8));
}

// The SHA-512 code builds a single block, so it is specified for messages of up to 13 words (104 bytes).
// The device post-processing hashes 8 words at a time. A longer message would need a second block.
static const int c_sha512_max_words = 13;

TEST(sha512_matches_known_answers_for_1_to_13_words) {
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	for (const Sha512Vector &vector : c_sha512_vectors) {
		if (vector.words > c_sha512_max_words) {
			continue;
		}
		std::vector<uint64_t> message((size_t)vector.words);
		for (int i = 0; i < vector.words; ++i) {
			message[(size_t)i] = sha512_word(i);
		}
		uint64_t digest[8] = {0};
		CHECK_EQ(api.sha512_generateHash(message.data(), (int16_t)vector.words, digest), 0);
		CHECK_EQ(hex_words(digest, 8), std::string(vector.digest));
	}
}

TEST(sha512_rejects_empty_and_oversized_messages) {
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	uint64_t message[16] = {1};
	uint64_t digest[8] = {0};
	CHECK_EQ(api.sha512_generateHash(message, 0, digest), -1);
	CHECK_EQ(api.sha512_generateHash(message, -1, digest), -1);
	CHECK_EQ(api.sha512_generateHash(message, 15, digest), -1);
	CHECK_EQ(api.sha512_generateHash(message, c_sha512_max_words, digest), 0);
}

TEST(sha512_is_repeatable) {
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	uint64_t message[10];
	for (int i = 0; i < 10; ++i) {
		message[i] = sha512_word(i);
	}
	uint64_t first[8] = {0};
	uint64_t second[8] = {0};
	REQUIRE(api.sha512_generateHash(message, 10, first) == 0);
	REQUIRE(api.sha512_generateHash(message, 10, second) == 0);
	CHECK_EQ(hex_words(first, 8), hex_words(second, 8));
}

TEST(sha256_serial_number_stamp_advances_for_each_block) {
	SwiftRngApi api;
	REQUIRE(api.is_context_initialized());
	uint32_t block[32] = {0};
	api.sha256_initializeSerialNumber(1000);
	api.sha256_stampSerialNumber(block);
	CHECK_EQ(block[8], (uint32_t)1000);
	api.sha256_stampSerialNumber(block);
	CHECK_EQ(block[8], (uint32_t)1001);
	api.sha256_stampSerialNumber(block);
	CHECK_EQ(block[8], (uint32_t)1002);
}
