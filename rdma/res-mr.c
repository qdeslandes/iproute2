// SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB
/*
 * res-mr.c	RDMA tool
 * Authors:     Leon Romanovsky <leonro@mellanox.com>
 */

#include "res.h"
#include <inttypes.h>

static int res_mr_line_raw(struct rd *rd, const char *name, int idx,
			   struct nlattr **nla_line)
{
	if (!nla_line[RDMA_NLDEV_ATTR_RES_RAW])
		return MNL_CB_ERROR;

	open_json_object(NULL);
	print_dev(idx, name);
	print_raw_data(rd, nla_line);
	close_json_object();
	newline();

	return MNL_CB_OK;
}

static int res_mr_line(struct rd *rd, const char *name, int idx,
		       struct nlattr **nla_line)
{
	uint32_t rkey = 0, lkey = 0;
	uint64_t iova = 0, mrlen;
	char *comm = NULL;
	uint32_t pdn = 0;
	uint32_t mrn = 0;
	uint32_t pid = 0;
	SPRINT_BUF(b);

	if (!nla_line[RDMA_NLDEV_ATTR_RES_MRLEN])
		return MNL_CB_ERROR;

	if (nla_line[RDMA_NLDEV_ATTR_RES_RKEY])
		rkey = mnl_attr_get_u32(nla_line[RDMA_NLDEV_ATTR_RES_RKEY]);
	if (nla_line[RDMA_NLDEV_ATTR_RES_LKEY])
		lkey = mnl_attr_get_u32(nla_line[RDMA_NLDEV_ATTR_RES_LKEY]);
	if (nla_line[RDMA_NLDEV_ATTR_RES_IOVA])
		iova = mnl_attr_get_u64(nla_line[RDMA_NLDEV_ATTR_RES_IOVA]);

	mrlen = mnl_attr_get_u64(nla_line[RDMA_NLDEV_ATTR_RES_MRLEN]);
	if (rd_is_filtered_attr(rd, "mrlen", mrlen,
				nla_line[RDMA_NLDEV_ATTR_RES_MRLEN]))
		goto out;

	if (nla_line[RDMA_NLDEV_ATTR_RES_PID]) {
		pid = mnl_attr_get_u32(nla_line[RDMA_NLDEV_ATTR_RES_PID]);
		if (!get_task_name(pid, b, sizeof(b)))
			comm = b;
	} else if (nla_line[RDMA_NLDEV_ATTR_RES_KERN_NAME]) {
		/* discard const from mnl_attr_get_str */
		comm = (char *)mnl_attr_get_str(
			nla_line[RDMA_NLDEV_ATTR_RES_KERN_NAME]);
	}

	if (rd_is_filtered_attr(rd, "pid", pid,
				nla_line[RDMA_NLDEV_ATTR_RES_PID]))
		goto out;

	if (nla_line[RDMA_NLDEV_ATTR_RES_MRN])
		mrn = mnl_attr_get_u32(nla_line[RDMA_NLDEV_ATTR_RES_MRN]);
	if (rd_is_filtered_attr(rd, "mrn", mrn,
				nla_line[RDMA_NLDEV_ATTR_RES_MRN]))
		goto out;

	if (nla_line[RDMA_NLDEV_ATTR_RES_PDN])
		pdn = mnl_attr_get_u32(nla_line[RDMA_NLDEV_ATTR_RES_PDN]);
	if (rd_is_filtered_attr(rd, "pdn", pdn,
				nla_line[RDMA_NLDEV_ATTR_RES_PDN]))
		goto out;

	open_json_object(NULL);
	print_dev(idx, name);
	res_print_u32("mrn", mrn, nla_line[RDMA_NLDEV_ATTR_RES_MRN]);
	print_key("rkey", rkey, nla_line[RDMA_NLDEV_ATTR_RES_RKEY]);
	print_key("lkey", lkey, nla_line[RDMA_NLDEV_ATTR_RES_LKEY]);
	print_key("iova", iova, nla_line[RDMA_NLDEV_ATTR_RES_IOVA]);
	res_print_u64("mrlen", mrlen, nla_line[RDMA_NLDEV_ATTR_RES_MRLEN]);
	res_print_u32("pdn", pdn, nla_line[RDMA_NLDEV_ATTR_RES_PDN]);
	res_print_u32("pid", pid, nla_line[RDMA_NLDEV_ATTR_RES_PID]);
	print_comm(comm, nla_line);

	print_driver_table(rd, nla_line[RDMA_NLDEV_ATTR_DRIVER]);
	print_raw_data(rd, nla_line);
	close_json_object();
	newline();

out:
	return MNL_CB_OK;
}

int res_mr_idx_parse_cb(const struct nlmsghdr *nlh, void *data)
{
	struct nlattr *tb[RDMA_NLDEV_ATTR_MAX] = {};
	struct rd *rd = data;
	const char *name;
	uint32_t idx;

	mnl_attr_parse(nlh, 0, rd_attr_cb, tb);
	if (!tb[RDMA_NLDEV_ATTR_DEV_INDEX] || !tb[RDMA_NLDEV_ATTR_DEV_NAME])
		return MNL_CB_ERROR;

	name = mnl_attr_get_str(tb[RDMA_NLDEV_ATTR_DEV_NAME]);
	idx = mnl_attr_get_u32(tb[RDMA_NLDEV_ATTR_DEV_INDEX]);

	return (rd->show_raw) ? res_mr_line_raw(rd, name, idx, tb) :
		res_mr_line(rd, name, idx, tb);
}

int res_mr_parse_cb(const struct nlmsghdr *nlh, void *data)
{
	struct nlattr *tb[RDMA_NLDEV_ATTR_MAX] = {};
	struct nlattr *nla_table, *nla_entry;
	struct rd *rd = data;
	int ret = MNL_CB_OK;
	const char *name;
	uint32_t idx;

	mnl_attr_parse(nlh, 0, rd_attr_cb, tb);
	if (!tb[RDMA_NLDEV_ATTR_DEV_INDEX] || !tb[RDMA_NLDEV_ATTR_DEV_NAME] ||
	    !tb[RDMA_NLDEV_ATTR_RES_MR])
		return MNL_CB_ERROR;

	name = mnl_attr_get_str(tb[RDMA_NLDEV_ATTR_DEV_NAME]);
	idx = mnl_attr_get_u32(tb[RDMA_NLDEV_ATTR_DEV_INDEX]);
	nla_table = tb[RDMA_NLDEV_ATTR_RES_MR];

	mnl_attr_for_each_nested(nla_entry, nla_table) {
		struct nlattr *nla_line[RDMA_NLDEV_ATTR_MAX] = {};

		ret = mnl_attr_parse_nested(nla_entry, rd_attr_cb, nla_line);
		if (ret != MNL_CB_OK)
			break;

		ret = (rd->show_raw) ? res_mr_line_raw(rd, name, idx, nla_line) :
			res_mr_line(rd, name, idx, nla_line);
		if (ret != MNL_CB_OK)
			break;
	}
	return ret;
}
